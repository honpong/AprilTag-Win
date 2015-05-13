#include "stdafx.h"
#include "SensorManager.h"
#include "Debug.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"

using namespace RealityCap;

static const int IMAGE_WIDTH = 640;
static const int IMAGE_HEIGHT = 480;
static const int FPS = 30;

SensorManager::SensorManager() : _isVideoStreaming(false)
{
	senseMan = PXCSenseManager::CreateInstance();
	if (!senseMan) Debug::Log(L"Unable to create the PXCSenseManager\n");
}

SensorManager::~SensorManager()
{
	senseMan->Release();
}

bool SensorManager::StartVideo()
{
	if (isVideoStreaming()) return true;
	if (!senseMan) return false;

	pxcStatus status;
	status = senseMan->EnableStream(PXCCapture::STREAM_TYPE_COLOR, IMAGE_WIDTH, IMAGE_HEIGHT, (pxcF32)FPS);
	if (status < PXC_STATUS_NO_ERROR)
	{
		Debug::Log(L"Failed to enable video stream(s)\n");
		return false;
	}

	status = senseMan->Init();
	if (status < PXC_STATUS_NO_ERROR)
	{
		Debug::Log(L"Failed to initialize video pipeline\n");
		return false;
	}
	
	_isVideoStreaming = true;

	// poll for frames in a separate thread
	videoThread = std::thread(&SensorManager::PollForFrames, this);

	return true;
}

void SensorManager::StopVideo()
{
	if (!isVideoStreaming()) return;
	_isVideoStreaming = false;
	videoThread.join();
	if (senseMan) senseMan->Close();
}

bool SensorManager::isVideoStreaming()
{
	return _isVideoStreaming;
}

void SensorManager::PollForFrames()
{
	while (isVideoStreaming() && senseMan->AcquireFrame(true) == PXC_STATUS_NO_ERROR)
	{
		PXCCapture::Sample *sample = senseMan->QuerySample();
		//if (sample) sampleHandler->OnNewSample(senseMan->CUID, sample);
		senseMan->ReleaseFrame();
	}
}

