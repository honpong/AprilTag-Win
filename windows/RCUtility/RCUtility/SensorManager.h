#pragma once

#using <Windows.winmd>
#using <Platform.winmd>

#include "IMUManager.h"
#include "VideoManager.h"

namespace RealityCap
{

	public ref class SensorManager sealed
	{
	public:
		SensorManager();
		bool StartSensors();
		void StopSensors();
		bool isRunning();

	private:
		class VideoFrameHandler : public PXCSenseManager::Handler
		{
		public:
			VideoFrameHandler(SensorManager^ parent);
			virtual pxcStatus PXCAPI OnNewSample(pxcUID, PXCCapture::Sample *sample);
		private:
			SensorManager^ _parent;
		};

		VideoFrameHandler frameHandler;
		VideoManager videoMan;
		IMUManager^ imuMan;
		bool _isRunning;

		void OnAmeterSample(Windows::Devices::Sensors::AccelerometerReading^ sample);
		void OnGyroSample(Windows::Devices::Sensors::GyrometerReading^ sample);
		void OnVideoFrame(PXCImage* colorSample);
	};
}