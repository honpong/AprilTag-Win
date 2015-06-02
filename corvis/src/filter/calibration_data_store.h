#pragma once
#include "device_parameters.h"
#include <memory>

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
            Always use this method to get an instance of a calibration_data_store;
        */
        static unique_ptr<calibration_data_store> GetStore();
        /*
            Loads the saved calibration. Check HasCalibration() before calling this method. Returns true if successful.
        */
        virtual bool LoadCalibration(corvis_device_parameters &cal) { return false; };
        /*
            Loads the default calibration for the specified device. Returns true if successful.
        */
        virtual bool LoadCalibrationDefaults(const corvis_device_type deviceType, corvis_device_parameters &cal) { return false; };
        /*
            Saves the calibration to the data store. Returns true if successful.
        */
        virtual bool SaveCalibration(const corvis_device_parameters &calibration) { return false; };
        /*
            Deletes the calibration from the data store.
        */
        virtual bool ClearCalibration() { return false; };
        /*
            Returns true if calibration data exists.
        */
        virtual bool HasCalibration() { return false; };
    };
}