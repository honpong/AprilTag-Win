#pragma once
#include "ICalibrationDataStore.h"

#define CALIBRATION_VERSION 7
#define CALIBRATION_FILE_NAME "calibration.json"

namespace RealityCap
{
    class CalibrationDataStore : public ICalibrationDataStore
    {
    public:
        CalibrationDataStore();
        ~CalibrationDataStore();
        virtual corvis_device_parameters GetSavedCalibration();
        virtual void SaveCalibration(corvis_device_parameters calibration);
        virtual void ClearCalibration();
        virtual bool HasCalibration();
    };
}
