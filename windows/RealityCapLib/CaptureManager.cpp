#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include "CaptureManager.h"
#include "Debug.h"
#include <string>
#include "libpxcimu_internal.h"

using namespace RealityCap;
using namespace std;

CaptureManager::CaptureManager() : SensorManager(), _isCapturing(false)
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
	_isCapturing = cp.start(_captureFilePath.c_str());
	return isCapturing();
}

void CaptureManager::StopCapture()
{
	if (!isCapturing()) return;
	cp.stop();
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
	auto data = camera_data(colorSample);
	cp.receive_camera(std::move(data));
}

void CaptureManager::OnAmeterSample(imu_sample_t* sample)
{
	if (!isCapturing()) return;

    accelerometer_data data;
    //windows gives acceleration in g-units, so multiply by standard gravity in m/s^2
    data.accel_m__s2[0] = -sample->data[0] * 9.80665;
    data.accel_m__s2[1] = sample->data[1] * 9.80665;
    data.accel_m__s2[2] = -sample->data[2] * 9.80665;
    data.timestamp = sensor_clock::time_point(sensor_clock::duration(sample->coordinatedUniversalTime100ns));
    cp.receive_accelerometer(std::move(data));
}

void CaptureManager::OnGyroSample(imu_sample_t* sample)
{
	if (!isCapturing()) return;

    gyro_data data;
    //windows gives angular velocity in degrees per second
    data.angvel_rad__s[0] = sample->data[0] * M_PI / 180.;
    data.angvel_rad__s[1] = sample->data[1] * M_PI / 180.;
    data.angvel_rad__s[2] = sample->data[2] * M_PI / 180.;
    data.timestamp = sensor_clock::time_point(sensor_clock::duration(sample->coordinatedUniversalTime100ns));
    cp.receive_gyro(std::move(data));
}

