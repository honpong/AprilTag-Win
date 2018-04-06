#pragma once

#include "rc_tracker.h"
#include "MotionAPI.h"
#include "MotionDeviceDescriptor.h"

namespace rc {
    static struct motion_device_initializer {
        motion_device_initializer() {
            for (auto &d : motion::motionDevices)
                motion::MotionDeviceManager::instance()->AddMotionDeviceDescriptor(d);
        }
    } mdi_;
    struct sensor_listener : protected motion::MotionDeviceListner
    {
        rc_Tracker *rc;
        motion::MotionDevice *md;
        rc_Sensor accel_id, gyro_id, camera_id;
      public:
        operator bool() { return !!md; }
        sensor_listener(rc_Tracker *rc_, int base_sensor_id, int motion_instance) : rc(rc_),
            accel_id(base_sensor_id), gyro_id(base_sensor_id), camera_id(base_sensor_id) {
            md = motion::MotionDeviceManager::instance()->connect(this, motion_instance); if (!md) return;
        }
        template <typename ForwardIt>
        static void start(ForwardIt begin, ForwardIt end) {
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

            for(ForwardIt it = begin; it != end; ++it)
                (*it)->md->start(profile);

            for(ForwardIt it = begin; it != end; ++it)
                (*it)->md->startFisheyeCamera();
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
                    case motion::MOTION_SOURCE_GYRO:  rc_receiveGyro         (rc, gyro_id,  frame[i].header.timestamp/1000, rc_Vector{frame[i].x, frame[i].y, frame[i].z}); break;
                    case motion::MOTION_SOURCE_ACCEL: rc_receiveAccelerometer(rc, accel_id, frame[i].header.timestamp/1000, rc_Vector{frame[i].x, frame[i].y, frame[i].z}); break;
                }
        }

        void fisheyeCallback(motion::MotionFisheyeFrame *f)
        {
            struct ctx { motion::MotionDevice *md; motion::MotionFisheyeFrame *f; };
            auto exposure_us = std::max<uint64_t>(0, f->exposure*1000); /* ms to us */
            rc_receiveImage(rc, camera_id, rc_FORMAT_GRAY8,
                            f->header.timestamp/1000 /* ns to us */ - exposure_us/2, exposure_us,
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
