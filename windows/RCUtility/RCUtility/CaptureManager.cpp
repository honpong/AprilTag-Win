#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "CaptureManager.h"
#include "Debug.h"
#include <string>

#using <Windows.winmd>
#using <Platform.winmd>

using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;
using namespace std;

CaptureManager::VideoFrameHandler::VideoFrameHandler(CaptureManager^ capMan)
{
	parent = capMan;
}

pxcStatus PXCAPI CaptureManager::VideoFrameHandler::OnNewSample(pxcUID, PXCCapture::Sample *sample)
{
	if (sample)
	{
		PXCImage* colorSample = sample->color;
		if (colorSample) parent->OnVideoFrame(colorSample);
	}

	return PXC_STATUS_NO_ERROR; // return NO ERROR to continue, or any ERROR to exit the loop
};

CaptureManager::CaptureManager() : frameHandler(this), _isCapturing(false)
{
	imuMan = ref new IMUManager();
	imuMan->OnAmeterSample += ref new AmeterEventHandler(this, &CaptureManager::OnAmeterSample);
	imuMan->OnGyroSample += ref new GyroEventHandler(this, &CaptureManager::OnGyroSample);

	videoMan.SetDelegate(&frameHandler);

	_captureFilePath = GetExePath() + "\\capture";
}

bool CaptureManager::StartSensors()
{
	bool result;
	result = videoMan.StartVideo();
	if (!result) return false;
	result = imuMan->StartSensors();
	return result;
}

void CaptureManager::StopSensors()
{
	imuMan->StopSensors();
	videoMan.StopVideo();
}

bool CaptureManager::StartCapture()
{
	if (isCapturing()) return true;
	_isCapturing = cp.start(_captureFilePath.c_str());
	return isCapturing();
}

void CaptureManager::StopCapture()
{
	if (!isCapturing()) return;
	cp.stop();
	_isCapturing = false;
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

void CaptureManager::OnAmeterSample(AccelerometerReading^ sample)
{
	if (!isCapturing()) return;

    accelerometer_data data;
    //windows gives acceleration in g-units, so multiply by standard gravity in m/s^2
    data.accel_m__s2[0] = sample->AccelerationX * 9.80665;
    data.accel_m__s2[1] = sample->AccelerationY * 9.80665;
    data.accel_m__s2[2] = sample->AccelerationZ * 9.80665;
    data.timestamp = sensor_clock::now();
    cp.receive_accelerometer(std::move(data));

    std::cerr << "accel timestamp: " << sample->Timestamp.UniversalTime << "\n";
    long long millisec = sample->Timestamp.UniversalTime / 10000;
	Debug::Log(L"%lld\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", millisec, sample->AccelerationX, sample->AccelerationY, sample->AccelerationZ);
}

void CaptureManager::OnGyroSample(GyrometerReading^ sample)
{
	if (!isCapturing()) return;

    gyro_data data;
    //windows gives angular velocity in degrees per second
    data.angvel_rad__s[0] = sample->AngularVelocityX * M_PI / 180.;
    data.angvel_rad__s[1] = sample->AngularVelocityY * M_PI / 180.;
    data.angvel_rad__s[2] = sample->AngularVelocityZ * M_PI / 180.;
    data.timestamp = sensor_clock::now();
	cp.receive_gyro(std::move(data));

    std::cerr << "gyro timestamp: " << sample->Timestamp.UniversalTime << "\n";
    long long millisec = sample->Timestamp.UniversalTime / 10000;
    Debug::Log(L"%lld\tgyro\tx: %3.3f\ty: %3.3f\tz: %3.3f", millisec, sample->AngularVelocityX, sample->AngularVelocityY, sample->AngularVelocityZ); // gyro data is in degrees/sec
}

void CaptureManager::OnVideoFrame(PXCImage* colorSample)
{
	if (!isCapturing()) return;
	//cp.receive_camera(camera_data(colorSample));
	Debug::Log(L"Color video sample received");
}

