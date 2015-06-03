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

std::unique_ptr<CalibrationManager> RCFactory::CreateCalibrationManager()
{
	return std::unique_ptr<CalibrationManager>(new CalibrationManager(CreateSenseManager()));
}

std::unique_ptr<CaptureManager> RCFactory::CreateCaptureManager()
{
    return std::unique_ptr<CaptureManager>(new CaptureManager(CreateSenseManager()));
}
