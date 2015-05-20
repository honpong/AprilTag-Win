#pragma once

#include "SensorManager.h"

namespace RealityCap
{
    class CalibrationManagerDelegate
    {
    protected:
        CalibrationManagerDelegate() {};
    public:
        virtual void OnProgressUpdated(float progress) {};
        virtual void OnStatusUpdated(int status) {};
    };

    class CalibrationManager : virtual public SensorManager
    {
    public:
        CalibrationManager(PXCSenseManager* senseMan);
        bool StartCalibration();
        void StopCalibration();
        bool isCalibrating();
        void SetDelegate(CalibrationManagerDelegate* del);

    protected:
        virtual void OnColorFrame(PXCImage* colorImage) override;
        virtual void OnAmeterSample(struct imu_sample* sample) override;
        virtual void OnGyroSample(struct imu_sample* sample) override;

    private:
        bool _isCalibrating;
        CalibrationManagerDelegate* _delegate;
    };
}