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
}

bool CaptureManager::StartCapture()
{
    if (isCapturing()) return true;
    std::wstring _captureFilePath = TEXT("capture.rssdk");
    _isCapturing = SetRecording(_captureFilePath.c_str());
    if (!_isCapturing) return false;
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

