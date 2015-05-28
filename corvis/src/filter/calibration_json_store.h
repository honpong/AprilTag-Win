#pragma once
#include "calibration_data_store.h"

#define CALIBRATION_FILE_NAME "calibration.json"
#define CALIBRATION_DEFAULTS_FILE_NAME "calibration-defaults.json"

namespace RealityCap
{
    class calibration_json_store : public calibration_data_store
    {
    public:
        calibration_json_store();
        ~calibration_json_store();
        /*
        Gets the saved calibration, or if none exists, the default calibration for the current device.
        */
        virtual corvis_device_parameters GetCalibration() override;
        /*
        Saves the calibration to a JSON file.
        */
        virtual void SaveCalibration(const corvis_device_parameters &calibration) override;
        virtual void SaveCalibration(const corvis_device_parameters &calibration, const char* fileName);
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
