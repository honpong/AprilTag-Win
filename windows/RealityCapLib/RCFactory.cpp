#include "stdafx.h"
#include "RCFactory.h"
#include "pxcsensemanager.h"

using namespace RealityCap;

RCFactory::RCFactory() : _senseMan(NULL)
{
}

RCFactory::~RCFactory()
{
    if (_senseMan) _senseMan->Release();
}

PXCSenseManager* RCFactory::CreateSenseManager()
{
    if (!_senseMan) _senseMan = PXCSenseManager::CreateInstance();
    return _senseMan;
}

std::unique_ptr<TrackerManager> RCFactory::CreateTrackerManager()
{
	return std::unique_ptr<TrackerManager>(new TrackerManager(CreateSenseManager()));
}

std::unique_ptr<SensorManager> RCFactory::CreateSensorManager()
{
    return std::unique_ptr<SensorManager>(new SensorManager(CreateSenseManager()));
}
