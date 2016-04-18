#pragma once

#include "rc_tracker.h"
#include "MotionAPI.h"

namespace rc {
    struct sensor_listener : protected motion::MotionListner
    {
        rc_Tracker *rc;
        motion::MotionDevice *md;

      public:
        operator bool() { return !!md; }
        sensor_listener(rc_Tracker *rc_) : rc(rc_) {
            md = motion::getMotionDeviceManagerInstance()->connect(this);
            if (!md) return;
            md->SetIMUCallbackBufferDepth(1);
            md->SetFisheyeBufferDepth(20);
            //md->SetAntiFlickerRate();
            //md->SetExposureLimits(float ExposureSecMin, float ExposureSecMax, float GainMin, float GainMax);
            md->StartData(MOTION_COMPONENT_ACCEL|MOTION_COMPONENT_GYRO|MOTION_COMPONENT_FISHEYE|MOTION_COMPONENT_DEPTH);
            md->StartTimestamps(MOTION_COMPONENT_DEPTH); // FIXME
        }
        ~sensor_listener() {
            if (!md) return;
            md->StopTimestamps();
            md->StopData();
            motion::getMotionDeviceManagerInstance()->disconnect(md);
        }

      protected:
        void sensorCallback(motion::SensorFrame* frame, uint32_t numFrames)
        {
            for (int i=0; i< numFrames; i++)
                switch(frame[i].type) {
                    case motion::MOTION_SENSOR_GYRO:  rc_receiveGyro         (rc, frame[i].timestamp/1000, rc_Vector{frame[i].x, frame[i].y, frame[i].z}); break;
                    case motion::MOTION_SENSOR_ACCEL: rc_receiveAccelerometer(rc, frame[i].timestamp/1000, rc_Vector{frame[i].x, frame[i].y, frame[i].z}); break;
                }
        }

        void fisheyeCallback(int slot, uint64_t timestamp)
        {
            motion::FisheyeFrame *f = md->GetFisheyeFrameDescriptor(slot);
            struct ctx { motion::MotionDevice *md; int slot; };
            rc_receiveImage(rc, f->timestamp/1000, f->frameExposureNs/1000, rc_FORMAT_GRAY8, // NOTE: f->timestamp == timestamp
                            f->frameWidth, f->frameHeight, f->frameStrideInBytes, f->frameData,
                            [](void*ctx_){ auto ctx = (struct ctx*)ctx_; ctx->md->ReturnFisheyeBuffer(ctx->slot); delete ctx; }, (void*)new ctx{md, slot});
        }

        void timestampCallback(uint32_t source, uint32_t frame_num, uint64_t timestamp_ns)
        {
            //std::cout << "source = " << source << " frame_num " << frame_num << " timestamp_ns "  << timestamp_ns << "\n";
        }
    };
}
