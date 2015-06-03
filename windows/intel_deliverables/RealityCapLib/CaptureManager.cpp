#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include "CaptureManager.h"
#include "Debug.h"
#include <string>
#include "libpxcimu_internal.h"

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
    return isCapturing();
}

void CaptureManager::StopCapture()
{
    if (!isCapturing()) return;
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
}

void CaptureManager::OnAmeterSample(imu_sample_t* sample)
{
    if (!isCapturing()) return;
}

void CaptureManager::OnGyroSample(imu_sample_t* sample)
{
    if (!isCapturing()) return;
}

