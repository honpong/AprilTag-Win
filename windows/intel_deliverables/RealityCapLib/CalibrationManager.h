#pragma once

#include "SensorManager.h"
#include <thread>

#include "rc_intel_interface.h"

namespace RealityCap
{
    class CalibrationManagerDelegate
    {
    protected:
        CalibrationManagerDelegate() {};
    public:
        virtual void OnProgressUpdated(float progress) {};
        virtual void OnStatusUpdated(int status) {};
        virtual void OnError(int code) {};
    };

    class CalibrationManager : virtual public SensorManager
    {
    public:
        CalibrationManager(PXCSenseManager* senseMan);
        virtual ~CalibrationManager();
        bool StartCalibration();
        void StopCalibration();
        bool isCalibrating();
        void SetDelegate(CalibrationManagerDelegate* del);
        void UpdateStatus(rc_TrackerState newState, rc_TrackerError errorCode, rc_TrackerConfidence confidence, float progress);

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(imu_sample_t* sample) override;
        virtual void OnGyroSample(imu_sample_t* sample) override;

    private:
        bool _isCalibrating;
        CalibrationManagerDelegate* _delegate;
        rc_Tracker* _tracker;
        int _trackerState;
    };
}