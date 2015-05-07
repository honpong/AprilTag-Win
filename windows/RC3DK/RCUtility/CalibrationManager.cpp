#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "CalibrationManager.h"
#include "Debug.h"
#include <string>

#using <Windows.winmd>
#using <Platform.winmd>

using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;
using namespace std;

CalibrationManager::CalibrationManager() : _isCalibrating(false)
{
	sensorMan = ref new SensorManager();
}

bool CalibrationManager::StartCalibration()
{
	if (isCalibrating()) return true;
	bool sensorsStarted = sensorMan->StartSensors();
	if (!sensorsStarted) return false;
	_isCalibrating = true;
	_delegate->OnStatusUpdated(1);
	return isCalibrating();
}

void CalibrationManager::StopCalibration()
{
	if (!isCalibrating()) return;
	sensorMan->StopSensors();
	_isCalibrating = false;
}

bool RealityCap::CalibrationManager::isCalibrating()
{
	return _isCalibrating;
}

void RealityCap::CalibrationManager::SetDelegate(IntPtr del)
{
	_delegate = (CalibrationManagerDelegate*)(void*)del;
}


