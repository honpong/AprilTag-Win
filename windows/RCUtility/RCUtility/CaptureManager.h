#pragma once

#using <Windows.winmd>
#using <Platform.winmd>

#include "IMUManager.h"
#include "VideoManager.h"

namespace RealityCap
{
	public ref class CaptureManager sealed
	{
	public:
		CaptureManager();
		bool StartCapture();
		void StopCapture();

	private:
		class VideoFrameHandler : public PXCSenseManager::Handler
		{
		public:
			VideoFrameHandler(CaptureManager^ capMan);
			virtual pxcStatus PXCAPI OnNewSample(pxcUID, PXCCapture::Sample *sample);
		private:
			CaptureManager^ parent;
		};
		VideoFrameHandler frameHandler;
		VideoManager videoMan;

		IMUManager^ imuMan;
		void OnAmeterSample(Windows::Devices::Sensors::AccelerometerReading^ sample);
		void OnGyroSample(Windows::Devices::Sensors::GyrometerReading^ sample);
	};
}