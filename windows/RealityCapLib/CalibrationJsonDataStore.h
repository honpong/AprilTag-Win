#pragma once
#include "ICalibrationDataStore.h"

#define CALIBRATION_FILE_NAME "calibration.json"
#define CALIBRATION_DEFAULTS_FILE_NAME "calibration-defaults.json"

namespace RealityCap
{
    class CalibrationJsonDataStore : public ICalibrationDataStore
    {
    public:
        CalibrationJsonDataStore();
        ~CalibrationJsonDataStore();
        /*
        Gets the saved calibration, or if none exists, the default calibration for the current device.
        */
        virtual corvis_device_parameters GetCalibration() override;
        /*
        Saves the calibration to a JSON file.
        */
        virtual void SaveCalibration(corvis_device_parameters calibration) override;
        /*
        Deletes the calibration JSON file. Returns the result of the remove() function; zero if successful.
        */
        virtual int ClearCalibration() override;
        /*
        Returns true if a calibration JSON file exists and is of the current version.
        */
        virtual bool HasCalibration() override;
    };
}
