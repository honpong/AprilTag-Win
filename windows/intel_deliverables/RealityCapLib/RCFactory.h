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
        PXCSenseManager* GetSenseManager() { return CreateSenseManager(); };

    private:
        PXCSenseManager* _senseMan;

        PXCSenseManager* CreateSenseManager();
    };
}
