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
		void StartCapture();
		void StopCapture();

	private:
		class SampleHandler : public PXCSenseManager::Handler
		{
		public:
			SampleHandler(CaptureManager^ capMan);
			virtual pxcStatus PXCAPI OnNewSample(pxcUID, PXCCapture::Sample *sample);
		private:
			CaptureManager^ parent;
		};
		SampleHandler sampleHandler;
		VideoManager videoMan;

		IMUManager^ imuMan;
		void OnAmeterSample(AmeterSample^ sample);
		void OnGyroSample(Windows::Devices::Sensors::GyrometerReading^ sample);
	};
}