#pragma once

#include "SensorManager.h"
#include <thread>

#include "rc_intel_interface.h"

namespace RealityCap
{
    class ReplayManagerDelegate
    {
    protected:
        ReplayManagerDelegate() {};
    public:
        virtual void OnError(int code) {};
        virtual void OnStatusUpdated(int status) {};
        virtual void OnDataUpdated(rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count) {};
    };

    class ReplayManager : virtual public SensorManager
    {
    public:
        ReplayManager(PXCSenseManager* senseMan);
        virtual ~ReplayManager();
        bool StartReplay(const wchar_t * filename);
        void StopReplay();
        void SetDelegate(ReplayManagerDelegate * del);
        void StatusCallback(rc_TrackerState newState, rc_TrackerError errorCode, rc_TrackerConfidence confidence, float progress);
        void DataCallback(rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count);

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(imu_sample_t* sample) override;
        virtual void OnGyroSample(imu_sample_t* sample) override;

    private:
        rc_Tracker* _tracker;
        int _trackerState;
        ReplayManagerDelegate * _delegate;
    };
}
