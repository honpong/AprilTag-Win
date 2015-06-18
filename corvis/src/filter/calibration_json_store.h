#pragma once
#include "calibration_data_store.h"
#include <string>

#define CALIBRATION_FILE_NAME "calibration.json"

namespace RealityCap
{
    class calibration_json_store : public calibration_data_store
    {
    public:
        static unique_ptr<calibration_data_store> GetStore() = delete; // TODO: there's probabaly a better design that doesn't require this
        calibration_json_store();
        virtual ~calibration_json_store();

        static bool SerializeCalibration(const corvis_device_parameters &cal, std::string &jsonString);
        static bool DeserializeCalibration(const std::string &jsonString, corvis_device_parameters &cal);
        
        virtual bool LoadCalibrationDefaults(const corvis_device_type deviceType, corvis_device_parameters &cal) override;
    };
}
