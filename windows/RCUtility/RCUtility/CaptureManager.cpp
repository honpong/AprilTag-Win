#include "stdafx.h"
#include "CaptureManager.h"
#include "Debug.h"
#include <string>

#using <Windows.winmd>
#using <Platform.winmd>

using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;

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

CaptureManager::CaptureManager() : frameHandler(this)
{
	imuMan = ref new IMUManager();
	imuMan->OnAmeterSample += ref new AmeterEventHandler(this, &CaptureManager::OnAmeterSample);
	imuMan->OnGyroSample += ref new GyroEventHandler(this, &CaptureManager::OnGyroSample);

	videoMan.SetDelegate(&frameHandler);
}

bool CaptureManager::StartSensors()
{
	bool result;
	result = imuMan->StartSensors();
	if (!result) return result;
	result = videoMan.StartVideo();
	return result;
}

void CaptureManager::StopSensors()
{
	imuMan->StopSensors();
	videoMan.StopVideo();
}

bool CaptureManager::StartCapture()
{
	std::string path = "C:\\Users\\ben_000\\Documents\\Visual Studio 2015\\Projects\\rcmain\\windows\\RCUtility\\Debug\\";
	return cp.start(path.c_str());
}

void CaptureManager::StopCapture()
{
	cp.stop();
}

void CaptureManager::OnAmeterSample(AccelerometerReading^ sample)
{
	cp.receive_accelerometer(accelerometer_data(sample));

	long long millisec = sample->Timestamp.UniversalTime / 10000;
	Debug::Log(L"%lld\taccel\tx: %1.3f\ty: %1.3f\tz: %1.3f", millisec, sample->AccelerationX, sample->AccelerationY, sample->AccelerationZ);
}

void CaptureManager::OnGyroSample(GyrometerReading^ sample)
{
	cp.receive_gyro(gyro_data(sample));

	long long millisec = sample->Timestamp.UniversalTime / 10000;
	Debug::Log(L"%lld\tgyro\tx: %3.3f\ty: %3.3f\tz: %3.3f", millisec, sample->AngularVelocityX, sample->AngularVelocityY, sample->AngularVelocityZ); // gyro data is in degrees/sec
}

void CaptureManager::OnVideoFrame(PXCImage* colorSample)
{
	cp.receive_camera(camera_data(colorSample));
	Debug::Log(L"Color video sample received");
}

