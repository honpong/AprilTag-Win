#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "CalibrationManager.h"
#include "Debug.h"
#include <string>
#include "libpxcimu_internal.h"
#include "rc_intel_interface.h"
#include "calibration_data_store.h"

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

    unique_ptr<calibration_data_store> store = calibration_data_store::GetStore();
    corvis_device_parameters cal;
    store->GetCalibrationDefaults(DEVICE_TYPE_UNKNOWN, &cal);
    rc_setCalibration(_tracker, cal);

    rc_startCalibration(_tracker);

    _pollingThread = std::thread(&CalibrationManager::PollForStatusUpdates, this);

    _isCalibrating = true;
    return isCalibrating();
}

void CalibrationManager::StopCalibration()
{
    if (!isCalibrating()) return;
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
    const rc_Vector vec = { 
        sample->data[0] * 9.80665,
        sample->data[1] * 9.80665,
        sample->data[2] * 9.80665
    };
    rc_receiveAccelerometer(_tracker, sample->coordinatedUniversalTime100ns, vec);
}

void CalibrationManager::OnGyroSample(imu_sample_t* sample)
{
    if (!isCalibrating()) return;
    const rc_Vector vec = {
        sample->data[0] * M_PI / 180.,
        sample->data[1] * M_PI / 180.,
        sample->data[2] * M_PI / 180.
    };
    rc_receiveAccelerometer(_tracker, sample->coordinatedUniversalTime100ns, vec);
}

void CalibrationManager::PollForStatusUpdates()
{
    while (isCalibrating())
    {
        rc_TrackerState newState = rc_getState(_tracker);
        if (newState != _trackerState)
        {
            if (_delegate) _delegate->OnStatusUpdated(newState);
            _trackerState = newState;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

