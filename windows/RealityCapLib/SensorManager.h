#pragma once

#include "stdafx.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include <thread>

namespace RealityCap
{
	class SensorManager
	{
	public:
		SensorManager();
		~SensorManager();
		
		bool StartVideo(); // returns true if video started successfully
		void StopVideo();
		void SetDelegate(PXCSenseManager::Handler* handler); // must be called before StartVideo()
		bool isVideoStreaming();

	private:		
		PXCSenseManager* senseMan;
		PXCSenseManager::Handler* sampleHandler;
		std::thread videoThread;
		void PollForFrames();
		bool _isVideoStreaming;
	};
}