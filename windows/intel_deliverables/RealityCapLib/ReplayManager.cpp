#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "ReplayManager.h"
#include "Debug.h"
#include <string>
#include <fstream>
#include <streambuf>
#include "libpxcimu_internal.h"
#include "rc_intel_interface.h"
#include "rc_pxc_util.h"

using namespace RealityCap;
using namespace std;

ReplayManager::ReplayManager(PXCSenseManager* senseMan) : SensorManager(senseMan), _delegate(NULL)
{
    _tracker = rc_create();
}

RealityCap::ReplayManager::~ReplayManager()
{
    rc_destroy(_tracker);
}

static void data_callback(void *handle, rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count)
{
    ((ReplayManager *)handle)->DataCallback(time, pose, features, feature_count);
}

static void status_callback(void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress)
{
    ((ReplayManager *)handle)->StatusCallback(state, error, confidence, progress);
}

const rc_Pose camera_pose = { 0, -1, 0, 0,
                             -1, 0, 0, 0,
                              0, 0, -1, 0 };

bool ReplayManager::StartReplay(const wchar_t * filename)
{
    Debug::Log(TEXT("Starting replay"));
    if (filename)
    {
        if (!StartPlayback(filename)) {
            Debug::Log(TEXT("StartPlayback failed"));
            return false;
        }
    } else {
        if (!StartSensors()) { // Live
            Debug::Log(TEXT("Live failed"));
            return false;
        }
    }

    std::wifstream t("gigabyte_s11.json");
    std::wstring calibrationJSON((std::istreambuf_iterator<wchar_t>(t)),
        std::istreambuf_iterator<wchar_t>());
    if (calibrationJSON.length() == 0) {
        Debug::Log(L"Couldn't open calibration, failing");
        return false;
    }
    rc_setCalibration(_tracker, calibrationJSON.c_str());

    PXCCaptureManager *capMan = GetSenseManager()->QueryCaptureManager();
    PXCCapture::Device *pDevice = capMan->QueryDevice();
    PXCPointF32 focal = pDevice->QueryColorFocalLength();
    PXCPointF32 principal = pDevice->QueryColorPrincipalPoint();
    pxcI32 exposure = pDevice->QueryColorExposure();
    rc_configureCamera(_tracker, rc_EGRAY8, camera_pose, 640, 480, principal.x, principal.y, focal.x, 0, focal.y);

    _trackerState = rc_E_INACTIVE;
    rc_setStatusCallback(_tracker, status_callback, this);
    rc_setDataCallback(_tracker, data_callback, this);

    rc_startTracker(_tracker);

    return true;
}

void ReplayManager::StopReplay()
{
    const wchar_t* buffer;
    size_t size = rc_getCalibration(_tracker, &buffer);
    std::wofstream t("calibration_result.json");
    t << std::wstring(buffer);
    t.close();
    rc_stopTracker(_tracker);
    StopSensors();
}

void ReplayManager::OnColorFrame(PXCImage* colorSample)
{
    //TODO: should this be new?
    RCSavedImage *si = new RCSavedImage(colorSample);
    int shutter_time_us = 0;
    //Timestamp: divide by 10 to go from 100ns to us, subtract 637us blank interval, subtract shutter time to get start of capture
    rc_receiveImage(_tracker, rc_EGRAY8, colorSample->QueryTimeStamp() / 10 - 637 - shutter_time_us, shutter_time_us, NULL, false, si->data.pitches[0], si->data.planes[0], RCSavedImage::releaseOpaquePointer, (void*)si);
}

void ReplayManager::OnAmeterSample(imu_sample_t* sample)
{
    const rc_Vector vec = rc_convertAcceleration(sample);
    rc_receiveAccelerometer(_tracker, sample->coordinatedUniversalTime100ns / 10, vec);
}

void ReplayManager::OnGyroSample(imu_sample_t* sample)
{
    const rc_Vector vec = rc_convertAngularVelocity(sample);
    rc_receiveGyro(_tracker, sample->coordinatedUniversalTime100ns / 10, vec);
}

void ReplayManager::SetDelegate(ReplayManagerDelegate* del)
{
    _delegate = del;
}

void ReplayManager::StatusCallback(rc_TrackerState newState, rc_TrackerError errorCode, rc_TrackerConfidence confidence, float progress)
{
    // check for errors
    if (errorCode && _delegate) _delegate->OnError(errorCode);

    // check for status changes
    if (newState != _trackerState)
    {
        if (_delegate) _delegate->OnStatusUpdated(newState);
            _trackerState = newState;
    }
}

void ReplayManager::DataCallback(rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count)
{
    if(_delegate)
        _delegate->OnDataUpdated(time, pose, features, feature_count);
}
