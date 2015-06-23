#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "TrackerManager.h"
#include "Debug.h"
#include <string>
#include <fstream>
#include <streambuf>
#include "libpxcimu_internal.h"
#include "rc_intel_interface.h"
#include "rc_pxc_util.h"

using namespace RealityCap;
using namespace std;

TrackerManager::TrackerManager(PXCSenseManager* senseMan) : SensorManager(senseMan), _isRunning(false), _delegate(NULL)
{
    _tracker = rc_create();
}

RealityCap::TrackerManager::~TrackerManager()
{
    rc_destroy(_tracker);
}

static void data_callback(void *handle, rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count)
{
    ((TrackerManager *)handle)->UpdateData(time, pose, features, feature_count);
}

static void status_callback(void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress)
{
    ((TrackerManager *)handle)->UpdateStatus(state, error, confidence, progress);
}

const rc_Pose camera_pose = { 0, -1, 0, 0,
                             -1, 0, 0, 0,
                              0, 0, -1, 0 };

void TrackerManager::ConfigureCameraIntrinsics()
{
    PXCCaptureManager *capMan = GetSenseManager()->QueryCaptureManager();
    PXCCapture::Device *pDevice = capMan->QueryDevice();
    PXCPointF32 focal = pDevice->QueryColorFocalLength();
    PXCPointF32 principal = pDevice->QueryColorPrincipalPoint();
    rc_configureCamera(_tracker, rc_EGRAY8, camera_pose, 640, 480, principal.x, principal.y, focal.x, 0, focal.y);
}

bool TrackerManager::ReadCalibration(std::wstring filename)
{
    std::wifstream t(filename);
    std::wstring calibrationJSON((std::istreambuf_iterator<wchar_t>(t)),
        std::istreambuf_iterator<wchar_t>());
    if (calibrationJSON.length() == 0) {
        Debug::Log(L"Couldn't load calibration %s", filename.c_str());
        return false;
    }
    bool result = rc_setCalibration(_tracker, calibrationJSON.c_str());
    if (!result) return false;

    return true;
}

bool TrackerManager::LoadDefaultCalibration()
{
    return ReadCalibration(GetExePath() + L"\\gigabyte_s11.json");
}

bool TrackerManager::LoadStoredCalibration()
{
    return ReadCalibration(GetExePath() + L"\\stored_calibration.json");
}

bool TrackerManager::WriteCalibration(std::wstring filename)
{
    const wchar_t* buffer;
    size_t size = rc_getCalibration(_tracker, &buffer);
    if (!size) return false;
    std::wofstream t(filename);
    t << std::wstring(buffer);
    t.close();
    if (t.bad()) return false;
    return true;
}

bool TrackerManager::StoreCurrentCalibration()
{
    return WriteCalibration(GetExePath() + L"\\stored_calibration.json");
}

bool TrackerManager::Start()
{
    if (isRunning()) return false;
    if (!LoadStoredCalibration())
        if (!LoadDefaultCalibration())
            return false;
    if (!StartSensors()) return false;

    _trackerState = rc_E_INACTIVE;
    _progress = 0.;
    
    rc_setStatusCallback(_tracker, status_callback, this);
    rc_setDataCallback(_tracker, data_callback, this);
    rc_startTracker(_tracker);

    _isRunning = true;

    return isRunning();
}

bool TrackerManager::StartCalibration()
{
    if (isRunning()) return false;
    if (!LoadDefaultCalibration()) return false;
    if (!StartSensors()) return false;

    //override the default calibration data with the device-specific camera intrinsics
    ConfigureCameraIntrinsics();

    _trackerState = rc_E_INACTIVE;
    _progress = 0.;

//    rc_setStatusCallback(_tracker, status_callback, this);
//    rc_setDataCallback(_tracker, data_callback, this);
    rc_startCalibration(_tracker);

    _isRunning = true;

    return isRunning();
}

void TrackerManager::SetOutputLog(const std::wstring filename)
{
    rc_setOutputLog(_tracker, filename.c_str());
}

bool TrackerManager::StartReplay(const std::wstring filename, bool realtime)
{
    if (isRunning()) return false;
    // Try to load filename.json, then look for calibration.json in
    // the same folder. If that doesn't work, fall back to loading the
    // default calibration
    std::wstring calibration_filename = filename + L".json";
    if (!ReadCalibration(calibration_filename)) {
        wstring::size_type pos = filename.find_last_of(L"\\/");
        if (pos >= filename.length()) calibration_filename = L".";
        else calibration_filename = wstring(filename.begin(), filename.begin() + pos);
        calibration_filename += L"\\calibration.json";
        if(!ReadCalibration(calibration_filename)) {
            Debug::Log(L"Reading default calibration");
            if(!LoadDefaultCalibration())
                return false;
        }
    }

    _trackerState = rc_E_INACTIVE;
    _progress = 0.;

    rc_setStatusCallback(_tracker, status_callback, this);
    rc_setDataCallback(_tracker, data_callback, this);
    rc_startTracker(_tracker);

    if (!StartPlayback(filename.c_str(), realtime)) {
        rc_stopTracker(_tracker);
        return false;
    }

    _isRunning = true;

    return isRunning();
}

void TrackerManager::Stop()
{
    if (!isRunning()) return;
    StopSensors();
    rc_stopTracker(_tracker);
    rc_reset(_tracker, 0, rc_POSE_IDENTITY);
    _isRunning = false;
}

bool TrackerManager::isRunning()
{
    return _isRunning;
}

void TrackerManager::SetDelegate(TrackerManagerDelegate* del)
{
    _delegate = del;
}

void TrackerManager::OnColorFrame(PXCImage* colorSample)
{
    RCSavedImage *si = new RCSavedImage(colorSample);
    PXCCaptureManager *capMan = GetSenseManager()->QueryCaptureManager();
    PXCCapture::Device *pDevice = capMan->QueryDevice();
    pxcI32 exposure = pDevice->QueryColorExposure();
    uint64_t shutter_time_us = 1 << exposure;
    PXCImage::ImageInfo info = colorSample->QueryInfo();
    //Timestamp: divide by 10 to go from 100ns to us, subtract 637us blank interval and 12 ms ad-hoc (tuning) offset, subtract shutter time to get start of capture
    rc_receiveImage(_tracker, rc_EGRAY8, colorSample->QueryTimeStamp() / 10 - 637 - 12000 - shutter_time_us, shutter_time_us, NULL, false, info.width, info.height, si->data.pitches[0], si->data.planes[0], RCSavedImage::releaseOpaquePointer, (void*)si);
}

void TrackerManager::OnAmeterSample(imu_sample_t* sample)
{
    if (!isRunning()) return;
    const rc_Vector vec = rc_convertAcceleration(sample);
    rc_receiveAccelerometer(_tracker, sample->coordinatedUniversalTime100ns / 10, vec);
    UpdateStatus(rc_getState(_tracker), rc_getError(_tracker), rc_getConfidence(_tracker), rc_getProgress(_tracker));
}

void TrackerManager::OnGyroSample(imu_sample_t* sample)
{
    if (!isRunning()) return;
    const rc_Vector vec = rc_convertAngularVelocity(sample);
    rc_receiveGyro(_tracker, sample->coordinatedUniversalTime100ns / 10, vec);
}

void TrackerManager::UpdateStatus(rc_TrackerState newState, rc_TrackerError errorCode, rc_TrackerConfidence confidence, float newProgress)
{
    if (newProgress != _progress)
    {
        if (_delegate) _delegate->OnProgressUpdated(newProgress);
        _progress = newProgress;
    }

    // check for errors
    if (errorCode && _delegate) _delegate->OnError(errorCode);

    // check for status changes
    if (newState != _trackerState)
    {
        if (_delegate) _delegate->OnStatusUpdated(newState);
        _trackerState = newState;
    }
}

void TrackerManager::UpdateData(rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count)
{
    if (_delegate)
        _delegate->OnDataUpdated(time, pose, features, feature_count);
}

wstring TrackerManager::GetExePath()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string buf(buffer);
    string::size_type pos = buf.find_last_of("\\/");
    return wstring(buf.begin(), buf.begin() + pos);
}
