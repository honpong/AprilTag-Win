#include "usb.h"

#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <bsp.h>
#include <mv_types.h>
#include <rtems.h>

#include "rc_tracker.h"
static rc_Tracker * tracker_instance = nullptr;
#include "sensor_fusion.h"
const filter * sfm;

#define USE_FAST_PATH rc_RUN_FAST_PATH

#include "usb_definitions.h"
#include "usbpump_application_rtems_api.h"
#include "usbpumpdebug.h"

#include "packet_io.h"
#include "tm2_outcall.h"
#include "usbpump_vsc2app.h"

#include "mv_trace.h"

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

bool      isReplayRunning = false;
pthread_t thReplay        = {0};

void status_callback(void * handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence)
{
    printf("Status change: state %d error %d confidence %d\n", state, error, confidence);
}

void data_callback(void * handle, rc_Tracker * tracker, const rc_Data * data)
{
    UNUSED(handle);

    packet_rc_pose_t output_pose;

    // only output fast path poses when using the fast path
    if((rc_DATA_PATH_FAST == data->path && rc_RUN_FAST_PATH != USE_FAST_PATH) ||
       (rc_DATA_PATH_SLOW == data->path &&
        rc_RUN_NO_FAST_PATH != USE_FAST_PATH))
        return;

    rc_PoseVelocity     pv;
    rc_PoseAcceleration pa;
    rc_PoseTime         pose_time = rc_getPose(tracker, &pv, &pa, data->path);

    output_pose.header.type      = packet_rc_pose;
    output_pose.header.time      = pose_time.time_us;
    output_pose.header.sensor_id = 0;
    output_pose.header.bytes     = sizeof(packet_rc_pose_t);
    output_pose.pose             = pose_time.pose_m;
    output_pose.velocity         = pv;
    output_pose.acceleration     = pa;

    if(!usb_blocking_write(ENDPOINT_DEV_INT_OUT, (uint8_t *)&output_pose,
                           sizeof(packet_rc_pose_t)))
        printf("Error writing 6dof\n");
}

void * fnReplay(void * arg)
{
    bool stereo_configured = false;

    tracker_instance = rc_create();
    sfm              = &((sensor_fusion *)tracker_instance)->sfm;
    rc_setDataCallback(tracker_instance, data_callback, tracker_instance);
    rc_setStatusCallback(tracker_instance, status_callback, tracker_instance);
    rc_configureQueueStrategy(tracker_instance, rc_QUEUE_MINIMIZE_DROPS);

    uint16_t replay_params = 0;
    uint64_t lastTimestamp = 0;
    if(replay_params & REPLAY_REALTIME)
        printf("Starting replay: realtime or slower\n");
    else
        printf("Starting replay: as fast as possible\n");

    rtems_object_set_name(rtems_task_self(), __func__);
    while(1) {
        if(!isReplayRunning) break;

        packet_t * packet;
        if(!packet_io_read(&packet)) {
            printf("Error reading packet, exiting\n");
            break;
        }

        if(replay_params & REPLAY_REALTIME) {
            if(lastTimestamp == 0) lastTimestamp = packet->header.time;
            if(lastTimestamp < packet->header.time)
                usleep(packet->header.time - lastTimestamp);
            lastTimestamp = packet->header.time;
        }

        switch(packet->header.type) {
            case packet_calibration_json: {
                packet_calibration_json_t * json = (packet_calibration_json_t *)packet;
                bool success = rc_setCalibration(tracker_instance, (char *)json->data);
                if(!success)
                    printf("Error: Failed to set calibration!\n");

                rc_startTracker(tracker_instance, rc_RUN_SYNCHRONOUS | USE_FAST_PATH);
                packet_io_free(packet);
                break;
            }
            case packet_accelerometer: {
                START_EVENT(EV_SF_REC_ACCEL, 0);
                packet_accelerometer_t * accel =
                        (packet_accelerometer_t *)packet;
                const rc_Vector acceleration_m__s2 = {accel->a[0], accel->a[1],
                                                      accel->a[2]};
                rc_receiveAccelerometer(tracker_instance,
                                        accel->header.sensor_id,
                                        accel->header.time, acceleration_m__s2);
                packet_io_free(packet);
                END_EVENT(EV_SF_REC_ACCEL, 0);
                break;
            }
            case packet_gyroscope: {
                START_EVENT(EV_SF_REC_GYRO, 0);
                packet_gyroscope_t * gyro = (packet_gyroscope_t *)packet;
                const rc_Vector      angular_velocity_rad__s = {
                        gyro->w[0], gyro->w[1], gyro->w[2]};
                rc_receiveGyro(tracker_instance, gyro->header.sensor_id,
                               gyro->header.time, angular_velocity_rad__s);
                packet_io_free(packet);
                END_EVENT(EV_SF_REC_GYRO, 0);
                break;
            }
            case packet_image_raw: {
                packet_image_raw_t * image = (packet_image_raw_t *)packet;
                if(image->header.sensor_id != 0) {  // only mono for now
                    packet_io_free(packet);
                    break;
                }
                START_EVENT(EV_SF_REC_IMAGE, 0);
                rc_receiveImage(tracker_instance, image->header.sensor_id,
                                (rc_ImageFormat)image->format,
                                image->header.time, image->exposure_time_us,
                                image->width, image->height, image->stride,
                                image->data,
                                [](void * p) { packet_io_free((packet_t *)p); },
                                packet);
                END_EVENT(EV_SF_REC_IMAGE, 0);
                break;
            }
            case packet_stereo_raw: {
                if(!stereo_configured) {
                    rc_configureStereo(tracker_instance, 0, 1);
                    stereo_configured = true;
                }
                START_EVENT(EV_SF_REC_STEREO, 0);
                packet_stereo_raw_t * stereo = (packet_stereo_raw_t *)packet;
                rc_receiveStereo(
                        tracker_instance, stereo->header.sensor_id,
                        (rc_ImageFormat)stereo->format, stereo->header.time,
                        stereo->exposure_time_us, stereo->width, stereo->height,
                        stereo->stride1, stereo->stride2, stereo->data,
                        stereo->data + stereo->stride1 * stereo->height,
                        [](void * p) { packet_io_free((packet_t *)p); },
                        packet);
                END_EVENT(EV_SF_REC_STEREO, 0);
                break;
            }
            default:
                printf("Unrecognized data type %d\n", packet->header.type);
                packet_io_free(packet);
                break;
        }
    }

    printf("Destroying tracker and exiting\n");

    rc_stopTracker(tracker_instance);
    printf("Timing:\n%s\n", rc_getTimingStats(tracker_instance));
    rc_destroy(tracker_instance);
    isReplayRunning = false;
    return NULL;
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
    isReplayRunning                = true;
    s = pthread_create(&thReplay, &attr, &fnReplay, nullptr);
    if(s != 0) {
        printf("ERR: [usbStart] cannot start the Replay thread");
        isReplayRunning = false;
    }
    return 0;
}

void stopReplay()
{
    if(isReplayRunning) {
        isReplayRunning = false;
        packet_io_unblock_read();
        pthread_join(thReplay, NULL);
    }
}

int usbInit()
{
    void * r;

    r = UsbPump_Rtems_DataPump_Startup(&sg_DataPump_AppConfig);
    if(!r) {
        printf("ERR: [usbInit] UPF_DataPump_Startup() failed: %p\n", r);
        return 1;
    }

    usb_init();

    return 0;
}
