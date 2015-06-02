#pragma once
#include "calibration_data_store.h"

#define CALIBRATION_FILE_NAME "calibration.json"

namespace RealityCap
{
    class calibration_json_store : public calibration_data_store
    {
    public:
        static unique_ptr<calibration_data_store> GetStore() = delete; // TODO: there's probabaly a better design that doesn't require this
        calibration_json_store();
        virtual ~calibration_json_store();

        static std::wstring SerializeCalibration(struct corvis_device_parameters *cal);
        static corvis_device_parameters DeserializeCalibration(std::wstring jsonString);
        
        virtual bool LoadCalibration(struct corvis_device_parameters *cal) override;
        virtual bool LoadCalibrationDefaults(corvis_device_type deviceType, struct corvis_device_parameters *cal) override;
        virtual bool SaveCalibration(const corvis_device_parameters &calibration) override;
        virtual bool SaveCalibration(const corvis_device_parameters &calibration, const char* fileName); // TODO delete
        virtual bool ClearCalibration() override;
        virtual bool HasCalibration() override;
    };
}
