#pragma once
#include "calibration_data_store.h"
#include <string>

#define CALIBRATION_FILE_NAME "calibration.json"

namespace RealityCap
{
    class calibration_json_store : public calibration_data_store
    {
    private:
        string calibrationLocation{""};
        void ParseCalibrationFile(string fileName, corvis_device_parameters &cal);

    public:
        static unique_ptr<calibration_data_store> GetStore() = delete; // TODO: there's probabaly a better design that doesn't require this
        calibration_json_store();
        virtual ~calibration_json_store();

        static bool SerializeCalibration(const corvis_device_parameters &cal, std::string &jsonString);
        static bool DeserializeCalibration(const std::string &jsonString, corvis_device_parameters &cal);
        
        virtual bool LoadCalibration(corvis_device_parameters &cal) override;
        virtual bool LoadCalibrationDefaults(const corvis_device_type deviceType, corvis_device_parameters &cal) override;
        virtual bool SaveCalibration(const corvis_device_parameters &calibration) override;
        virtual bool SaveCalibration(const corvis_device_parameters &calibration, const char* fileName); // TODO delete
        virtual bool ClearCalibration() override;
        virtual bool HasCalibration() override;

        void SetCalibrationLocation(string location) { calibrationLocation = location; };
    };
}
