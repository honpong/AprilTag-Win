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
        ~RCFactory();
        unique_ptr<CalibrationManager> CreateCalibrationManager();
        unique_ptr<CaptureManager> CreateCaptureManager();

    private:
        PXCSenseManager* _senseMan;

        PXCSenseManager* CreateSenseManager();
    };
}
