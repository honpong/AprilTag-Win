#include "stdafx.h"
#include "RCFactory.h"
#include "pxcsensemanager.h"

using namespace RealityCap;

RCFactory::RCFactory()
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

unique_ptr<CalibrationManager> RCFactory::CreateCalibrationManager()
{
    return make_unique<CalibrationManager>(CreateSenseManager());
}

unique_ptr<CaptureManager> RCFactory::CreateCaptureManager()
{
    return make_unique<CaptureManager>(CreateSenseManager());
}
