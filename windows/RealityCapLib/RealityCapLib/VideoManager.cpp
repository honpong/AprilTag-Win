#include "pch.h"
#include "VideoManager.h"
#include "Debug.h"

using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;
using namespace Windows::UI::Xaml;

static const int IMAGE_WIDTH = 640;
static const int IMAGE_HEIGHT = 480;
static const int FPS = 30;

pxcStatus PXCAPI VideoManager::SampleHandler::OnNewSample(pxcUID, PXCCapture::Sample *sample)
{
	if (sample)
	{
		PXCImage* colorSample = sample->color;
		if (colorSample) Debug::Log(L"Color video sample received");
	}

	return PXC_STATUS_NO_ERROR; // return NO ERROR to continue, or any ERROR to exit the loop
};

VideoManager::VideoManager()
{
	senseMan = PXCSenseManager::CreateInstance();
	if (!senseMan) Debug::Log(L"Unable to create the PXCSenseManager\n");
}

VideoManager::~VideoManager()
{
	senseMan->Release();
}

//VideoManager^ VideoManager::GetSharedInstance()
//{
//	static VideoManager^ _sharedInstance;
//	if (_sharedInstance == nullptr)
//	{
//		Debug::Log(L"Instantiating shared VideoManager instance");
//		_sharedInstance = ref new VideoManager();
//	}
//	return _sharedInstance;
//}

bool VideoManager::StartVideo()
{
	if (!senseMan) return false;

	senseMan->EnableStream(PXCCapture::STREAM_TYPE_COLOR, IMAGE_WIDTH, IMAGE_HEIGHT, (pxcF32)FPS);

	/* Initializes the pipeline */
	pxcStatus status;
	status = senseMan->Init(&sampleHandler);

	if (status < PXC_STATUS_NO_ERROR)
	{
		Debug::Log(L"Failed to locate any video stream(s)\n");
		return false;
	}

	senseMan->StreamFrames(true);

	return true;
}

void VideoManager::StopVideo()
{
	if (senseMan) senseMan->Close();
}

