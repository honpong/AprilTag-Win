#using <Windows.winmd>
#using <Platform.winmd>

#include "stdafx.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"

#pragma once

namespace RealityCap
{
	public ref class VideoManager sealed
	{

	public:
		VideoManager();
		virtual ~VideoManager();
		//static VideoManager^ GetSharedInstance();
		bool StartVideo(); // returns true if video started successfully
		void StopVideo();

	private:
		class SampleHandler : public PXCSenseManager::Handler 
		{ 
		public:
			virtual pxcStatus PXCAPI OnNewSample(pxcUID, PXCCapture::Sample *sample); 
		};
		PXCSenseManager* senseMan;
		SampleHandler sampleHandler;
	};
}