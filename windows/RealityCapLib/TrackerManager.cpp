#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "TrackerManager.h"
#include "Debug.h"
#include <string>
#include <fstream>
#include <locale>
#include <codecvt>
#include <streambuf>
#include "libpxcimu_internal.h"
#include "rc_tracker.h"
#include "rc_pxc_util.h"
#include "pxcprojection.h"

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
    struct rc_CameraIntrinsics intrinsics = {};
    intrinsics.type = rc_CALIBRATION_TYPE_UNDISTORTED;
    intrinsics.width_px = 640;
    intrinsics.height_px = 480;
    intrinsics.f_x_px = focal.x;
    intrinsics.f_y_px = focal.y;
    intrinsics.c_x_px = principal.x;
    intrinsics.c_y_px = principal.y;
    rc_configureCamera(_tracker, rc_CAMERA_ID_COLOR, nullptr, &intrinsics);
}

bool TrackerManager::ReadCalibration(std::wstring filename)
{
    std::ifstream t(filename);
    std::string calibrationJSON((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
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
    const char* buffer;
    size_t size = rc_getCalibration(_tracker, &buffer);
    if (!size) return false;
    std::ofstream t(filename);
    t << std::string(buffer);
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
    rc_startTracker(_tracker, rc_E_SYNCRONOUS);

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
    rc_startCalibration(_tracker, rc_E_SYNCRONOUS);

    _isRunning = true;

    return isRunning();
}

void TrackerManager::SetOutputLog(const std::wstring filename)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    rc_setOutputLog(_tracker, converter.to_bytes(filename).c_str(), rc_E_SYNCRONOUS);
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
    rc_startTracker(_tracker, rc_E_SYNCRONOUS);

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
    RCSavedImage *si = new RCSavedImage(colorSample, PXCImage::PIXEL_FORMAT_Y8);
    PXCCaptureManager *capMan = GetSenseManager()->QueryCaptureManager();
    PXCCapture::Device *pDevice = capMan->QueryDevice();
    pxcI32 exposure = pDevice->QueryColorExposure();
    uint64_t shutter_time_us = 1 << exposure;
    PXCImage::ImageInfo info = colorSample->QueryInfo();
    //Timestamp: divide by 10 to go from 100ns to us, subtract 637us blank interval and 12 ms ad-hoc (tuning) offset, subtract shutter time to get start of capture
    rc_receiveImage(_tracker, colorSample->QueryTimeStamp() / 10 - 637 - 12000 - shutter_time_us, shutter_time_us, rc_FORMAT_GRAY8,
                    info.width, info.height, si->data.pitches[0], si->data.planes[0], RCSavedImage::releaseOpaquePointer, (void*)si);
}

void TrackerManager::OnColorFrameWithDepth(PXCImage* colorSample, PXCImage* depthSample)
{
	PXCCaptureManager *capMan = GetSenseManager()->QueryCaptureManager();
	PXCCapture::Device *pDevice = capMan->QueryDevice();

	RCSavedImage *sic = new RCSavedImage(colorSample, PXCImage::PIXEL_FORMAT_Y8);
	pxcI32 exposure = pDevice->QueryColorExposure();
	uint64_t shutter_time_us = 1 << exposure;
	PXCImage::ImageInfo colorInfo = colorSample->QueryInfo();

    RCSavedImage *sid = 0;
    PXCImage::ImageInfo depthInfo;

    PXCProjection *proj = pDevice->CreateProjection();
    PXCImage *remap_depth = proj->CreateDepthImageMappedToColor(depthSample, colorSample);
    if (remap_depth)
    {
        sid = new RCSavedImage(remap_depth, PXCImage::PIXEL_FORMAT_DEPTH);
        remap_depth->Release(); //release the reference we got for creating it; we just added another
        depthInfo = remap_depth->QueryInfo();
        //Timestamp: divide by 10 to go from 100ns to us, subtract 637us blank interval and 12 ms ad-hoc (tuning) offset, subtract shutter time to get start of capture
        rc_receiveImage(_tracker, colorSample->QueryTimeStamp() / 10 - 637 - 12000 - shutter_time_us, shutter_time_us, rc_FORMAT_DEPTH16,
                        depthInfo.width, depthInfo.height, sid->data.pitches[0], sid->data.planes[0], RCSavedImage::releaseOpaquePointer, (void*)sid);
    }
    rc_receiveImage(_tracker, colorSample->QueryTimeStamp() / 10 - 637 - 12000 - shutter_time_us, shutter_time_us, rc_FORMAT_GRAY8,
                    colorInfo.width, colorInfo.height, sic->data.pitches[0], sic->data.planes[0], RCSavedImage::releaseOpaquePointer, (void*)sic);
    proj->Release();

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
