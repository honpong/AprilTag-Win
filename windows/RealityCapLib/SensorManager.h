#pragma once

#include "stdafx.h"
#include <thread>

class PXCSenseManager;

namespace RealityCap
{
	class SensorManager
	{
	public:
		SensorManager();
		~SensorManager();
		
		bool StartSensors(); // returns true if video started successfully
		void StopSensors();
		bool isVideoStreaming();

	private:		
		PXCSenseManager* senseMan;
		std::thread videoThread;
		void PollForFrames();
		bool _isVideoStreaming;
	};
}