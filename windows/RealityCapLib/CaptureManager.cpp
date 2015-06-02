#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include "CaptureManager.h"
#include "Debug.h"
#include <string>
#include "libpxcimu_internal.h"
#include "SensorData.h"

using namespace RealityCap;
using namespace std;

CaptureManager::CaptureManager(PXCSenseManager* senseMan) : SensorManager(senseMan), _isCapturing(false)
{
    _captureFilePath = GetExePath() + "\\capture";
}

bool CaptureManager::StartCapture()
{
    if (isCapturing()) return true;
    if (!isVideoStreaming())
    {
        if (!StartSensors()) return false;
    }
    //_isCapturing = cp.start(_captureFilePath.c_str());
    return isCapturing();
}

void CaptureManager::StopCapture()
{
    if (!isCapturing()) return;
    //cp.stop();
    _isCapturing = false;
    StopSensors();
}

bool RealityCap::CaptureManager::isCapturing()
{
    return _isCapturing;
}

string CaptureManager::GetExePath()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string::size_type pos = string(buffer).find_last_of("\\/");
    return string(buffer).substr(0, pos);
}

void CaptureManager::OnColorFrame(PXCImage* colorSample)
{
    if (!isCapturing()) return;
    auto data = camera_data_from_PXCImage(colorSample);
    //cp.receive_camera(std::move(data));
}

void CaptureManager::OnAmeterSample(imu_sample_t* sample)
{
    if (!isCapturing()) return;

    accelerometer_data data = accelerometer_data_from_imu_sample_t(sample);
    //cp.receive_accelerometer(std::move(data));
}

void CaptureManager::OnGyroSample(imu_sample_t* sample)
{
    if (!isCapturing()) return;

    gyro_data data = gyro_data_from_imu_sample_t(sample);
    //cp.receive_gyro(std::move(data));
}

