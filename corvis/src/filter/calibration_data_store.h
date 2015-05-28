#pragma once
#include "device_parameters.h"

namespace RealityCap
{
    /*
        This interface can be implemented by any type of data store that might be used for calibration data. It allows us to easily change the implementation of the store from JSON to say, a database.
    */
    class calibration_data_store
    {
    protected:
        calibration_data_store() {};

    public:
        /*
            Gets the saved calibration, or if none exists, the default calibration for the current device.
        */
        virtual corvis_device_parameters GetCalibration() { return corvis_device_parameters(); };
        /*
            Saves the calibration to the data store.
        */
        virtual void SaveCalibration(corvis_device_parameters calibration) {};
        /*
            Deletes the calibration from the data store.
        */
        virtual int ClearCalibration() { return false; };
        /*
            Returns true if calibration data exists and is of the current version.
        */
        virtual bool HasCalibration() { return false; };
    };
}