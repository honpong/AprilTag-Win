#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <bsp.h>
#include <mv_types.h>
#include <rtems.h>
#include <deque>

#include "rc_tracker.h"
#include "tm2_playback_stream.h"
#include "usb.h"
#include "usb_definitions.h"
#include "usbpump_application_rtems_api.h"
#include "usbpumpdebug.h"
#include "packet_io.h"
#include "tm2_outcall.h"
#include "usbpump_vsc2app.h"
#include "replay_device.h"
#include "mv_trace.h"

#define USE_FAST_PATH rc_RUN_FAST_PATH

static pthread_t thReplay = { 0 };
static replay_device rp_device;

void * fnReplay(void * arg)
{
    rtems_object_set_name(rtems_task_self(), __func__);
    __attribute__((section(".cmx.bss"), aligned(64)))
    static uint8_t rc_tracker_memory[250 * 1024];
    static size_t rc_tracker_size = 0;
    if (rc_tracker_size || (rc_create_at(nullptr, &rc_tracker_size), rc_tracker_size > sizeof(rc_tracker_memory))) {
        printf("\e[31;1mWarning\e[m: reserved %td for rc_Tracker in CMX, but needed %td; using DDR\n", sizeof(rc_tracker_memory), rc_tracker_size);
        rp_device.init(tm2_pb_stream::get_instance(), { rc_create(), rc_destroy });
    } else
        rp_device.init(tm2_pb_stream::get_instance(), { rc_create_at(rc_tracker_memory, nullptr), rc_destroy });
    rp_device.start();
    printf("Destroying tracker and exiting\n");
    return 0;
}

int startReplay()
{
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
    rp_device.stop();
    pthread_join(thReplay, NULL);
}

int usbInit()
{
    tm2_pb_stream::get_instance()->init_device();
    return 0;
}
