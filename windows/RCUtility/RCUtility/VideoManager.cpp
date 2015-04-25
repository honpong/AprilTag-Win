#include "stdafx.h"
#include "VideoManager.h"
#include "Debug.h"

#using <Windows.winmd>
#using <Platform.winmd>

using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;
using namespace Windows::UI::Xaml;

static const int IMAGE_WIDTH = 640;
static const int IMAGE_HEIGHT = 480;
static const int FPS = 30;

VideoManager::VideoManager() : isVideoStreaming(false)
{
	senseMan = PXCSenseManager::CreateInstance();
	if (!senseMan) Debug::Log(L"Unable to create the PXCSenseManager\n");
}

VideoManager::~VideoManager()
{
	senseMan->Release();
}

bool VideoManager::StartVideo()
{
	if (!senseMan) return false;

	senseMan->EnableStream(PXCCapture::STREAM_TYPE_COLOR, IMAGE_WIDTH, IMAGE_HEIGHT, (pxcF32)FPS);

	/* Initializes the pipeline */
	pxcStatus status;
	status = senseMan->Init();

	if (status < PXC_STATUS_NO_ERROR)
	{
		Debug::Log(L"Failed to locate any video stream(s)\n");
		return false;
	}
	
	isVideoStreaming = true;

	// poll for frames in a separate thread
	videoThread = std::thread(&VideoManager::WaitForFrames, this);

	return true;
}

void VideoManager::StopVideo()
{
	isVideoStreaming = false;
	if (senseMan) senseMan->Close();
	videoThread.join();
}

void VideoManager::SetDelegate(PXCSenseManager::Handler* handler)
{
	sampleHandler = handler;
}

void VideoManager::WaitForFrames()
{
	while (isVideoStreaming && senseMan->AcquireFrame(true) == PXC_STATUS_NO_ERROR)
	{
		PXCCapture::Sample *sample = senseMan->QuerySample();
		if (sample) sampleHandler->OnNewSample(senseMan->CUID, sample);
		senseMan->ReleaseFrame();
	}
}

