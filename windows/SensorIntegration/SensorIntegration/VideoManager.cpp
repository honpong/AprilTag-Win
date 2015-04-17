#include "stdafx.h"
#include "VideoManager.h"
#include "Debug.h"
#using <Windows.winmd>
#using <Platform.winmd>
#include "pxcsensemanager.h"
#include "pxcmetadata.h"

using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;
using namespace Windows::UI::Xaml;

#define USE_WIN32_AMETER_API true
static const int DESIRED_FPS = 30;

VideoManager::VideoManager()
{
	pp = PXCSenseManager::CreateInstance();
	if (!pp) Debug::Log(L"Unable to create the PXCSenseManager\n");

	cm = pp->QueryCaptureManager();
	if (!cm) Debug::Log(L"Unable to create the PXCCaptureManager\n");
}

VideoManager^ VideoManager::GetSharedInstance()
{
	static VideoManager^ _sharedInstance;
	if (_sharedInstance == nullptr)
	{
		Debug::Log(L"Instantiating shared VideoManager instance");
		_sharedInstance = ref new VideoManager();
	}
	return _sharedInstance;
}

bool VideoManager::StartVideo()
{
	bool result = true;

	// TODO: spawn a thread for this
	pxcStatus sts;
	do {
		PXCVideoModule::DataDesc desc = {};
		if (cm->QueryCapture()) {
			cm->QueryCapture()->QueryDeviceInfo(0, &desc.deviceInfo);
		}
		else {
			desc.deviceInfo.streams = PXCCapture::STREAM_TYPE_COLOR; // | PXCCapture::STREAM_TYPE_DEPTH;
		}
		pp->EnableStreams(&desc);

		/* Initializes the pipeline */
		sts = pp->Init();

		if (sts<PXC_STATUS_NO_ERROR) {
			wprintf_s(L"Failed to locate any video stream(s)\n");
			break;
		}
		pp->QueryCaptureManager()->QueryDevice()->SetMirrorMode(PXCCapture::Device::MirrorMode::MIRROR_MODE_DISABLED);

		/* Stream Data */
		for (int nframes = 0; nframes < 50000; nframes++) {
			/* Waits until new frame is available and locks it for application processing */
			sts = pp->AcquireFrame(false);

			if (sts<PXC_STATUS_NO_ERROR) {
				if (sts == PXC_STATUS_STREAM_CONFIG_CHANGED) {
					wprintf_s(L"Stream configuration was changed, re-initilizing\n");
					pp->Close();
				}
				break;
			}

			const PXCCapture::Sample *sample = pp->QuerySample();
			if (sample) {
				Debug::Log(L"video sample received");
			}

			/* Releases lock so pipeline can process next frame */
			pp->ReleaseFrame();
		}
	} while (sts == PXC_STATUS_STREAM_CONFIG_CHANGED);

	return result;
}

void VideoManager::StopVideo()
{
	
}

