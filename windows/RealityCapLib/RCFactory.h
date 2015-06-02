#pragma once
#include "SensorManager.h"
#include "CalibrationManager.h"
#include "CaptureManager.h"
#include <memory>

using namespace std;

class PXCSenseManager;

namespace RealityCap
{
    class RCFactory
    {
    public:
        RCFactory();
        virtual ~RCFactory();
        unique_ptr<CalibrationManager> CreateCalibrationManager();
        unique_ptr<CaptureManager> CreateCaptureManager();
        PXCSenseManager* GetSenseManager() { return _senseMan; };

    private:
        PXCSenseManager* _senseMan;

        PXCSenseManager* CreateSenseManager();
    };
}
