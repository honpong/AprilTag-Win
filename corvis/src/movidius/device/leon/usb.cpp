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
    rp_device.init(tm2_pb_stream::get_instance());
    rtems_object_set_name(rtems_task_self(), __func__);
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
