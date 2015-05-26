#pragma once
#include "../../corvis/src/filter/device_parameters.h"

namespace RealityCap
{
    class ICalibrationDataStore
    {
    protected:
        ICalibrationDataStore() {};

    public:
        virtual corvis_device_parameters GetSavedCalibration() {};
        virtual void SaveCalibration(corvis_device_parameters calibration) {};
        virtual void ClearCalibration() {};
        virtual bool HasCalibration() {};
    }
}