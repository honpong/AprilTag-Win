#include <pthread.h>
#include <stdio.h>
#include <mv_types.h>
#include <rtems.h>
#include <memory>

#include "usb.h"
#include "usbpump_application_rtems_api.h"
#include "usbpumpdebug.h"
#include "tm2_outcall.h"
#include "replay_device.h"
#include "rc_tracker.h"
#include "tm2_playback_stream.h"

#ifndef DISABLE_LEON_DCACHE
#define USBPUMP_MDK_CACHE_ENABLE 1
#else
#define USBPUMP_MDK_CACHE_ENABLE 0
#endif

// USB VSC Handler
extern USBPUMP_VSC2APP_CONTEXT *               pSelf;

static USBPUMP_APPLICATION_RTEMS_CONFIGURATION sg_DataPump_AppConfig =
USBPUMP_APPLICATION_RTEMS_CONFIGURATION_INIT_V1(
    /* nEventQueue */ 64,
    /* pMemoryPool */ NULL,
    /* nMemoryPool */ 0,
    /* DataPumpTaskPriority */ 100,
    /* DebugTaskPriority */ 200,
    /* UsbInterruptPriority */ 10,
    /* pDeviceSerialNumber */ NULL,
    /* pUseBusPowerFn */ NULL,
    /* fCacheEnabled */ USBPUMP_MDK_CACHE_ENABLE,
    /* DebugMask */ UDMASK_ANY | UDMASK_ERRORS);

#define USE_FAST_PATH rc_RUN_FAST_PATH

static pthread_t thReplay = { 0 };
static std::unique_ptr<replay_device> rp_device;
static std::unique_ptr<tm2_pb_stream> tm2_stream;

void * fnReplay(void * arg)
{
    const size_t allocation_size = 250 * 1024;
    rtems_object_set_name(rtems_task_self(), __func__);
    __attribute__((section(".cmx.bss"), aligned(64)))
    static uint8_t rc_tracker_memory[allocation_size];
    memset(rc_tracker_memory, 0, allocation_size);
    size_t rc_tracker_size = 0;
    if ((rc_create_at(nullptr, &rc_tracker_size), rc_tracker_size > sizeof(rc_tracker_memory))) {
        printf("\e[31;1mWarning\e[m: reserved %td for rc_Tracker in CMX, but needed %td; using DDR\n", sizeof(rc_tracker_memory), rc_tracker_size);
        rp_device->init(tm2_stream.get(), { rc_create(), rc_destroy });
    } else
        rp_device->init(tm2_stream.get(), { rc_create_at(rc_tracker_memory, nullptr), rc_destroy });
    rp_device->start();
    printf("Destroying tracker and exiting\n");
    return 0;
}

int startReplay()
{
    rp_device = std::make_unique<replay_device>();
    tm2_stream = std::make_unique<tm2_pb_stream>();
    tm2_stream->init_device();
    int s = 0;
    pthread_attr_t attr;
    if(pthread_attr_init(&attr) != 0) {
        printf("pthread_attr_init error");
    }
    if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0) {
        printf("pthread_attr_setinheritsched error");
    }
    if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
        printf("pthread_attr_setschedpolicy error");
    }

    attr.schedparam.sched_priority = 2;
    s = pthread_create(&thReplay, &attr, &fnReplay, nullptr);
    if(s != 0) {
        printf("ERR: [usbStart] cannot start the Replay thread");
    }
    return 0;
}

void stopReplay()
{
    if (rp_device) rp_device->stop();
    rp_device = nullptr;
    tm2_stream = nullptr;
    pthread_join(thReplay, NULL);
}

int usbInit()
{
    void * r = UsbPump_Rtems_DataPump_Startup(&sg_DataPump_AppConfig);
    if (!r) {
        printf("ERR: [usbInit] UPF_DataPump_Startup() failed: %p\n", r);
        return false;
    }
    usb_init();
    return 0;
}
