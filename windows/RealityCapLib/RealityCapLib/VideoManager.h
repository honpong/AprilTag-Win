#pragma once
#ifndef VIDEOMANAGER_H
#define VIDEOMANAGER_H

#include "pch.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"

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

#endif