///
/// @file
/// @copyright (c) Intel Corp. 2017
///
/// @brief     Main leon file
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <bsp.h>
#include <bsp/irq.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h> /* For O_* constants */
#include <icb_defines.h>
#include <math.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

#include <rtems.h>
#include <rtems/cpuuse.h>
#include <rtems/libio.h>
#include <rtems/status-checks.h>

#include <DrvDdr.h>
#include <DrvGpio.h>
#include <DrvLeon.h>
#include <DrvLeonL2C.h>
#include <DrvShaveL2Cache.h>
#include <DrvTimer.h>
#include <OsCDCEL.h>
#include <OsDrvCmxDma.h>
#include <OsDrvCpr.h>
#include <OsDrvGpio.h>
#include <OsDrvShaveL2Cache.h>
#include <OsDrvSpiBus.h>
#include <OsDrvTimer.h>
#include <OsDrvTimer.h>
#include <OsDrvUsbPhy.h>

#include "rtems_config.h"

#include "blis.h"
#include "blis_defines.h"

#include "mv_trace.h"
#include "mv_types.h"

#include "clocks.h"

#include "usb.h"

#define CMX_CONFIG_SLICE_7_0 (0x11111111)
#define CMX_CONFIG_SLICE_15_8 (0x11111111)

#define BLIS_HEAP_SIZE (16 * 1024 * 1024)
unsigned char DDR_LEON_HEAP __attribute__((aligned(16))) g_blis_heap[BLIS_HEAP_SIZE];

CmxRamLayoutCfgType __attribute__((section(".cmx.ctrl")))
__cmx_config = {CMX_CONFIG_SLICE_7_0, CMX_CONFIG_SLICE_15_8};

#define L2CACHE_CFG (SHAVE_L2CACHE_NORMAL_MODE)

// BSP_SET_CLOCK(_refClock, _pll0TargetFreq, _masterNum, _masterDen, _cssClocks, _mssClocks, _upaClocks, _sippClocks, _auxClocks) \

BSP_SET_CLOCK(DEFAULT_OSC_CLOCK_KHZ, APP_CLOCK_KHZ, 1, 1, CSS_LOS_CLOCKS,
              MSS_CLOCKS, UPA_CLOCKS, SIPP_CLOCKS, 0);

//BSP_SET_L2C_CONFIG(_enable, _repl, _locked_ways, _mode, _count, _mtrrArray) 
BSP_SET_L2C_CONFIG(1, L2C_REPL_LRU, 0, L2C_MODE_WRITE_THROUGH, 0, NULL);

int initClocksAndMemory(void)
{
    s32 sc;

    // Configure the system
    sc = OsDrvCprInit();
    if(sc) return -sc;
    sc = OsDrvCprOpen();
    if(sc) return -sc;
    sc = OsDrvCprAuxClockArrayConfig(auxClkConfig);
    if(sc) return -sc;
    sc = OsDrvCprSetupClocks(&appClockConfig);
    if(sc) return -sc;

    sc = OsDrvCprSysDeviceAction(MSS_DOMAIN, DEASSERT_RESET, -1);
    if(sc) return -sc;
    sc = OsDrvCprSysDeviceAction(CSS_DOMAIN, DEASSERT_RESET, -1);
    if(sc) return -sc;
    sc = OsDrvCprSysDeviceAction(SIPP_DOMAIN, DEASSERT_RESET, -1);
    if(sc) return -sc;
    sc = OsDrvCprSysDeviceAction(UPA_DOMAIN, DEASSERT_RESET, -1);
    if(sc) return -sc;

    DrvCprAuxClocksEnable(AUX_CLK_MASK_GPIO | AUX_CLK_MASK_GPIO1, CLK_SRC_PLL0,
                          1, 25);
    DrvGpioSetMode(59, 3);  // left clock
    DrvGpioSetMode(57, 2);  // right clock
    DrvGpioSetMode(20, 4);  // SCL
    DrvGpioSetMode(21, 4);  // SDA
    //DrvGpioSetPwm()
    sc = OsDrvCmxDmaInit(DEFAULT_CDMA_INT_LEVEL, 14, 1, DMA_AGENT1);
    if(sc != OS_MYR_DRV_SUCCESS) {
        printf("OsDrvCmxDmaInitDefault failed with error: %ld\n", sc);
        return sc;
    }

    // TODO: Init and partition L2C Shave cache if we want to use it
    // (TM2 does this in leon_rt main since cache is disabled on leon)
    sc = OsDrvShaveL2CacheInit(SHAVEL2C_MODE_BYPASS);

    return RTEMS_SUCCESSFUL;
}

#include <dirent.h>

rtems_status_code brdInitializeBoard(void)
{
    rtems_status_code sc;
    struct stat       st;
    SET_REG_WORD(AON_RETENTION0_ADR,
                 GET_REG_WORD_VAL(AON_RETENTION0_ADR) | (1 << 23));
    OsDrvTimerInit();

    sc = initClocksAndMemory();
    if(sc) return -sc;

#if(TM2_BRD_REV == 0)
    DrvDdrInitialise(NULL);
#endif

    //DrvLL2CDisable(LL2C_OPERATION_INVALIDATE_AND_WRITE_BACK);

    return RTEMS_SUCCESSFUL;
}

// 2:  Source Specific #defines and types  (typedef, enum, struct)
// ----------------------------------------------------------------------------

osDrvUsbPhyParam_t initParam = {.enableOtgBlock = USB_PHY_OTG_DISABLED,
#if(TM2_BRD_REV == 0)
                                .useExternalClock = USB_PHY_USE_EXT_CLK,
#else
                                .useExternalClock = 0,
#endif
                                .fSel       = USB_REFCLK_20MHZ,
                                .refClkSel0 = USB_SUPER_SPEED_CLK_CONFIG};

#define CMX_NOCACHE __attribute__((section(".cmx_direct.data")))

#ifndef DISABLE_LEON_DCACHE
#define USBPUMP_MDK_CACHE_ENABLE 1
#else
#define USBPUMP_MDK_CACHE_ENABLE 0
#endif

extern u32 lrt_start;

extern rtems_id USBPumpMutex;

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

void POSIX_Init(void * args)
{
    (void)args;

    int sc = 0;

    /* Initialization and configuration */
    brdInitializeBoard();
    INIT_EVENTS();
    printf("Starting\n");

    OsDrvUsbPhyInit(&initParam);

    printf("INF: [Init] LeonOS started.\n");

    memset(g_blis_heap, 0, sizeof(g_blis_heap));
    bli_init_leon(g_blis_heap, BLIS_HEAP_SIZE);
    bli_init();

    if(usbInit() != 0) {
        printf("ERR: [Init] USB initialization failed\n");
        exit(0);
    }
    else {
        printf("INF: [Init] USB initialization done\n");
    }
}

// User extension to be able to catch abnormal terminations
static void Fatal_extension(Internal_errors_Source the_source, bool is_internal,
                            Internal_errors_t the_error)
{
    switch(the_source) {

        case RTEMS_FATAL_SOURCE_EXIT:
            if(the_error) {
                printk("Exited with error code %u\n", the_error);
            }
            break;  // normal exit
        case RTEMS_FATAL_SOURCE_ASSERT: {
            printk("%s : %d in %s \n",
                   ((rtems_assert_context *)the_error)->file,
                   ((rtems_assert_context *)the_error)->line,
                   ((rtems_assert_context *)the_error)->function);
        } break;
        case RTEMS_FATAL_SOURCE_EXCEPTION: {
            rtems_exception_frame_print(
                    (const rtems_exception_frame *)the_error);
        } break;
        default: {
            printk("\nSource %d Internal %d Error %u  0x%X:\n", the_source,
                   is_internal, the_error, (unsigned int)the_error);
        } break;
    }
}
