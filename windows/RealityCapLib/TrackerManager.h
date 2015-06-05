#pragma once

#include "SensorManager.h"
#include <thread>

#include "rc_intel_interface.h"

namespace RealityCap
{
    class TrackerManagerDelegate
    {
    protected:
        TrackerManagerDelegate() {};
    public:
        virtual void OnProgressUpdated(float progress) {};
        virtual void OnStatusUpdated(int status) {};
        virtual void OnError(int code) {};
    };

    class TrackerManager : virtual public SensorManager
    {
    public:
        TrackerManager(PXCSenseManager* senseMan);
        virtual ~TrackerManager();
        bool StartTracking();
        void StopTracking();
        bool isRunning();
        void SetDelegate(TrackerManagerDelegate* del);
        void UpdateStatus(rc_TrackerState newState, rc_TrackerError errorCode, rc_TrackerConfidence confidence, float progress);

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(imu_sample_t* sample) override;
        virtual void OnGyroSample(imu_sample_t* sample) override;

    private:
        bool _isRunning;
        TrackerManagerDelegate* _delegate;
        rc_Tracker* _tracker;
        int _trackerState;
    };
}