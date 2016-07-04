#pragma once

#include "rc_tracker.h"
#include "MotionAPI.h"
#include "MotionDeviceDescriptor.h"

namespace rc {
    struct sensor_listener : protected motion::MotionDeviceListner
    {
        rc_Tracker *rc;
        motion::MotionDevice *md;

        static class motion_device_initializer {
            motion_device_initializer() {
                for (auto &d : motion::motionDevices)
                    motion::MotionDeviceManager::instance()->AddMotionDeviceDescriptor(d);
            }
        } mdi_;
      public:
        operator bool() { return !!md; }
        sensor_listener(rc_Tracker *rc_, int instance) : rc(rc_) {
            md = motion::MotionDeviceManager::instance()->connect(this, instance); if (!md) return;

            motion::MotionProfile profile = {};
            profile.imuBufferDepth = 1;
            profile.fisheyeBufferDepth = 20;
            profile.syncIMU = false;

            profile.gyro.enable = true;
            profile.gyro.fps = motion::MotionProfile::Gyro::FPS_200;
            profile.gyro.range = motion::MotionProfile::Gyro::RANGE_1000;

            profile.accel.enable = true;
            profile.accel.fps = motion::MotionProfile::Accel::FPS_125;
            profile.accel.range = motion::MotionProfile::Accel::RANGE_4;

            profile.fisheye.enable = true;
            profile.fisheye.fps = motion::MotionProfile::Fisheye::FPS_30;

            profile.depth.enable = false;

            int ret = md->start(profile);

        }
        ~sensor_listener() {
            if (!md) return;
            md->stop();
            motion::MotionDeviceManager::instance()->disconnect(md);
        }

      protected:
        void sensorCallback(motion::MotionSensorFrame *frame, int numFrames)
        {
            for (int i=0; i< numFrames; i++)
                switch(frame[i].header.type) {
                    case motion::MOTION_SOURCE_GYRO:  rc_receiveGyro         (rc, frame[i].header.timestamp/1000, rc_Vector{frame[i].x, frame[i].y, frame[i].z}); break;
                    case motion::MOTION_SOURCE_ACCEL: rc_receiveAccelerometer(rc, frame[i].header.timestamp/1000, rc_Vector{frame[i].x, frame[i].y, frame[i].z}); break;
                }
        }

        void fisheyeCallback(motion::MotionFisheyeFrame *f)
        {
            struct ctx { motion::MotionDevice *md; motion::MotionFisheyeFrame *f; };
            rc_receiveImage(rc, f->header.timestamp/1000, f->exposure/1000, rc_FORMAT_GRAY8,
                            f->width, f->height, f->stride, f->data,
                            [](void*ctx_){ auto ctx = (struct ctx*)ctx_; ctx->md->returnFisheyeBuffer(ctx->f); delete ctx; },
                            (void*)new ctx{md, f});
        }

        void timestampCallback(motion::MotionTimestampFrame *ts)
        {
        }

        void notifyCallback(uint32_t status, uint8_t *buf, uint32_t size) {}
    };
}
