#pragma once
#include "SensorManager.h"
#include "CalibrationManager.h"
#include "CaptureManager.h"
#include <memory>

class PXCSenseManager;

namespace RealityCap
{
    class RCFactory
    {
    public:
        RCFactory();
        virtual ~RCFactory();
        std::unique_ptr<CalibrationManager> CreateCalibrationManager();
        std::unique_ptr<CaptureManager> CreateCaptureManager();
        PXCSenseManager* GetSenseManager() { return _senseMan; };

    private:
        PXCSenseManager* _senseMan;

        PXCSenseManager* CreateSenseManager();
    };
}
