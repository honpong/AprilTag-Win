#pragma once
#include "../../corvis/src/filter/device_parameters.h"

namespace RealityCap
{
    class ICalibrationDataStore
    {
    protected:
        ICalibrationDataStore() {};

    public:
        /*
            Gets the saved calibration, or if none exists, the default calibration for the current device.
        */
        virtual corvis_device_parameters GetCalibration() { return corvis_device_parameters(); };
        /*
            Saves the calibration to a JSON file.
        */
        virtual void SaveCalibration(corvis_device_parameters calibration) {};
        /*
            Deletes the calibration JSON file. Returns the result of the remove() function; zero if successful.
        */
        virtual int ClearCalibration() { return false; };
        /*
            Returns true if a calibration JSON file exists and is of the current version.
        */
        virtual bool HasCalibration() { return false; };
    };
}