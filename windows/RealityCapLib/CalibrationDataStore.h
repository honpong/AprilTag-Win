#pragma once
#include "ICalibrationDataStore.h"

#define CALIBRATION_FILE_NAME "calibration.json"
#define CALIBRATION_DEFAULTS_FILE_NAME "calibration-defaults.json"

namespace RealityCap
{
    class CalibrationDataStore : public ICalibrationDataStore
    {
    public:
        CalibrationDataStore();
        ~CalibrationDataStore();
        virtual corvis_device_parameters GetCalibration() override;
        virtual void SaveCalibration(corvis_device_parameters calibration) override;
        virtual int ClearCalibration() override;
        virtual bool HasCalibration() override;
    };
}
