#pragma once

#include "SensorManager.h"
#include <thread>

class rc_Tracker;

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

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(imu_sample_t* sample) override;
        virtual void OnGyroSample(imu_sample_t* sample) override;
        void PollForStatusUpdates();

    private:
        bool _isCalibrating;
        CalibrationManagerDelegate* _delegate;
        rc_Tracker* _tracker;
        std::thread _pollingThread;
        int _trackerState;
    };
}