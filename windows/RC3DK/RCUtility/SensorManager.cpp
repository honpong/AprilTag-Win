#include "stdafx.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "SensorManager.h"
#include "Debug.h"
#include <string>
#include "..\..\..\corvis\src\filter\capture.h"

using namespace Windows::Devices::Sensors;
using namespace RealityCap;
using namespace std;


SensorManager::VideoFrameHandler::VideoFrameHandler(SensorManager^ parent)
{
	_parent = parent;
}

pxcStatus PXCAPI SensorManager::VideoFrameHandler::OnNewSample(pxcUID, PXCCapture::Sample *sample)
{
	if (sample)
	{
		PXCImage* colorSample = sample->color;
		if (colorSample) _parent->OnVideoFrame(colorSample);
	}

	return PXC_STATUS_NO_ERROR; // return NO ERROR to continue, or any ERROR to exit the loop
};

SensorManager::SensorManager() : frameHandler(this), _isRunning(false)
{
	imuMan = ref new IMUManager();
	imuMan->OnAmeterSample += ref new AmeterEventHandler(this, &SensorManager::OnAmeterSample);
	imuMan->OnGyroSample += ref new GyroEventHandler(this, &SensorManager::OnGyroSample);

	videoMan.SetDelegate(&frameHandler);
}

bool SensorManager::StartSensors()
{
	bool result;
	result = videoMan.StartVideo();
	if (!result) return false;
	result = imuMan->StartSensors();
	_isRunning = result;
	return _isRunning;
}

void SensorManager::StopSensors()
{
	imuMan->StopSensors();
	videoMan.StopVideo();
	_isRunning = false;
}

bool SensorManager::isRunning()
{
	return _isRunning;
}

void SensorManager::OnAmeterSample(AccelerometerReading^ sample)
{
	if (!isRunning()) return;

	accelerometer_data data;
	//windows gives acceleration in g-units, so multiply by standard gravity in m/s^2
	data.accel_m__s2[0] = sample->AccelerationX * 9.80665;
	data.accel_m__s2[1] = sample->AccelerationY * 9.80665;
	data.accel_m__s2[2] = sample->AccelerationZ * 9.80665;
	data.timestamp = sensor_clock::now();

	// send data to filter

	std::cerr << "accel timestamp: " << sample->Timestamp.UniversalTime << "\n";
	long long millisec = sample->Timestamp.UniversalTime / 10000;
	Debug::Log(L"%lld\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", millisec, sample->AccelerationX, sample->AccelerationY, sample->AccelerationZ);
}

void SensorManager::OnGyroSample(GyrometerReading^ sample)
{
	if (!isRunning()) return;

	gyro_data data;
	//windows gives angular velocity in degrees per second
	data.angvel_rad__s[0] = sample->AngularVelocityX * M_PI / 180.;
	data.angvel_rad__s[1] = sample->AngularVelocityY * M_PI / 180.;
	data.angvel_rad__s[2] = sample->AngularVelocityZ * M_PI / 180.;
	data.timestamp = sensor_clock::now();

	// send data to filter

	std::cerr << "gyro timestamp: " << sample->Timestamp.UniversalTime << "\n";
	long long millisec = sample->Timestamp.UniversalTime / 10000;
	Debug::Log(L"%lld\tgyro\tx: %3.3f\ty: %3.3f\tz: %3.3f", millisec, sample->AngularVelocityX, sample->AngularVelocityY, sample->AngularVelocityZ); // gyro data is in degrees/sec
}

void SensorManager::OnVideoFrame(PXCImage* colorSample)
{
	if (!isRunning()) return;
	// send data to filter
	Debug::Log(L"Color video sample received");
}

