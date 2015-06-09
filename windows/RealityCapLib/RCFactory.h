#pragma once
#include "SensorManager.h"
#include "TrackerManager.h"
#include <memory>

class PXCSenseManager;

namespace RealityCap
{
    class RCFactory
    {
    public:
        RCFactory();
        virtual ~RCFactory();
        std::unique_ptr<SensorManager> CreateSensorManager();
        std::unique_ptr<TrackerManager> CreateTrackerManager();
        PXCSenseManager* GetSenseManager() { return _senseMan; };

    private:
        PXCSenseManager* _senseMan;

        PXCSenseManager* CreateSenseManager();
    };
}
