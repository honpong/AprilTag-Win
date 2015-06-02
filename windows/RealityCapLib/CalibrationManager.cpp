#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "CalibrationManager.h"
#include "Debug.h"
#include <string>
#include "libpxcimu_internal.h"
#include "rc_intel_interface.h"
#include "rc_pxc_util.h"

using namespace RealityCap;
using namespace std;

CalibrationManager::CalibrationManager(PXCSenseManager* senseMan) : SensorManager(senseMan), _isCalibrating(false), _delegate(NULL)
{
    _tracker = rc_create();
}

RealityCap::CalibrationManager::~CalibrationManager()
{
    rc_destroy(_tracker);
}

bool CalibrationManager::StartCalibration()
{
    if (isCalibrating()) return true;
    if (!StartSensors()) return false;

    _trackerState = rc_E_INACTIVE;

    rc_startCalibration(_tracker);

    _isCalibrating = true;

    _pollingThread = std::thread(&CalibrationManager::PollForStatusUpdates, this);

    return isCalibrating();
}

void CalibrationManager::StopCalibration()
{
    if (!isCalibrating()) return;
    rc_Calibration cal = rc_getCalibration(_tracker);
    // TODO: write calibration
    rc_stopTracker(_tracker);
    StopSensors();
    _isCalibrating = false;
}

bool CalibrationManager::isCalibrating()
{
    return _isCalibrating;
}

void CalibrationManager::SetDelegate(CalibrationManagerDelegate* del)
{
    _delegate = del;
}

void CalibrationManager::OnColorFrame(PXCImage* colorSample)
{
    // video not needed for calibration
}

void CalibrationManager::OnAmeterSample(imu_sample_t* sample)
{
    if (!isCalibrating()) return;
    const rc_Vector vec = rc_convertAcceleration(sample);
    rc_receiveAccelerometer(_tracker, sample->coordinatedUniversalTime100ns / 10, vec);
}

void CalibrationManager::OnGyroSample(imu_sample_t* sample)
{
    if (!isCalibrating()) return;
    const rc_Vector vec = rc_convertAngularVelocity(sample);
    rc_receiveGyro(_tracker, sample->coordinatedUniversalTime100ns / 10, vec);
}

void CalibrationManager::PollForStatusUpdates()
{
    while (isCalibrating())
    {
        // check for errors
        int errorCode = rc_getError(_tracker);
        if (errorCode && _delegate) _delegate->OnError(errorCode);

        // check for status changes
        rc_TrackerState newState = rc_getState(_tracker);
        if (newState != _trackerState)
        {
            if (_delegate) _delegate->OnStatusUpdated(newState);
            _trackerState = newState;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

