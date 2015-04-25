#pragma once

#using <Windows.winmd>
#using <Platform.winmd>

#include "stdafx.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include <thread>

namespace RealityCap
{
	class VideoManager
	{
	public:
		VideoManager();
		~VideoManager();
		
		bool StartVideo(); // returns true if video started successfully
		void StopVideo();
		void SetDelegate(PXCSenseManager::Handler* handler); // must be called before StartVideo()

	private:		
		PXCSenseManager* senseMan;
		PXCSenseManager::Handler* sampleHandler;
		std::thread videoThread;
		void WaitForFrames();
		bool isVideoStreaming;
	};
}