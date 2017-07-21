#pragma once

#include <rtems.h>
#include <rtems/status-checks.h>

#include <DrvCpr.h>

#define CSS_LOS_CLOCKS 0xFFFFFFFFFFFFFFFFULL
#define MSS_CLOCKS 0xFFFFFFFFUL
#define SIPP_CLOCKS 0xFFFFFFFFUL
#define UPA_CLOCKS 0xFFFFFFFFUL

#define USB_CLOCK_DIVIDER 30
#define APP_CLOCK_KHZ 600000
#if(TM2_BRD_REV == 0)
#define DEFAULT_OSC_CLOCK_KHZ 12000
#else
#define DEFAULT_OSC_CLOCK_KHZ 24000
#endif

static const tyAuxClkDividerCfg auxClkConfig[] = {
        {
                .auxClockEnableMask = AUX_CLK_MASK_MEDIA | AUX_CLK_MASK_CIF0 |
                                      AUX_CLK_MASK_CIF1,
                .auxClockSource         = CLK_SRC_SYS_CLK_DIV2,
                .auxClockDivNumerator   = 1,
                .auxClockDivDenominator = 1,
        },
        {
                .auxClockEnableMask =
                        AUX_CLK_MASK_MIPI_ECFG | AUX_CLK_MASK_MIPI_CFG,
                .auxClockSource         = CLK_SRC_SYS_CLK_DIV2,
                .auxClockDivNumerator   = 1,
                .auxClockDivDenominator = 15,  // gives 20MHz clock
        },
        {.auxClockEnableMask =
                 AUX_CLK_MASK_USB_PHY_EXTREFCLK | AUX_CLK_MASK_USB_CTRL_REF_CLK,
         .auxClockSource         = CLK_SRC_PLL0,
         .auxClockDivNumerator   = 1,
         .auxClockDivDenominator = USB_CLOCK_DIVIDER},
        {.auxClockEnableMask     = AUX_CLK_MASK_USB_CTRL_SUSPEND_CLK,
         .auxClockSource         = CLK_SRC_PLL0,
         .auxClockDivNumerator   = 1,
         .auxClockDivDenominator = USB_CLOCK_DIVIDER},
        {.auxClockEnableMask     = AUX_CLK_MASK_USB_PHY_REF_ALT_CLK,
         .auxClockSource         = CLK_SRC_PLL0,
         .auxClockDivNumerator   = 1,
         .auxClockDivDenominator = USB_CLOCK_DIVIDER},
        {.auxClockEnableMask     = AUX_CLK_MASK_GPIO | AUX_CLK_MASK_GPIO1,
         .auxClockSource         = CLK_SRC_PLL0,
         .auxClockDivNumerator   = 1,
         .auxClockDivDenominator = 25},
        {0, 0, 0, 0},  // Null Terminated List
};

static const tySocClockConfig appClockConfig = {
#if(TM2_BRD_REV == 0)
        .refClk0InputKhz = 12000,  // Default 12Mhz input clock for es0 boards
#else
        .refClk0InputKhz = 24000,  // Default 24Mhz input clock for es1 boards
#endif
        .refClk1InputKhz   = 0,  // Assume no secondary oscillator for now
        .targetPll0FreqKhz = APP_CLOCK_KHZ,
        .targetPll1FreqKhz = 0,                // set in DDR driver
        .clkSrcPll1        = CLK_SRC_REFCLK0,  // Supply both PLLS from REFCLK0
        .masterClkDivNumerator   = 1,
        .masterClkDivDenominator = 1,
        .cssDssClockEnableMask   = CSS_LOS_CLOCKS,
        .mssClockEnableMask      = MSS_CLOCKS,
        .sippClockEnableMask     = SIPP_CLOCKS,
        .upaClockEnableMask      = UPA_CLOCKS,
        .pAuxClkCfg              = auxClkConfig,
};
