#using <Windows.winmd>
#using <Platform.winmd>

#pragma once

namespace RealityCap
{
	public ref class VideoManager sealed
	{
	public:
		VideoManager();
		static VideoManager^ GetSharedInstance();
		bool StartVideo(); // returns true if video started successfully
		void StopVideo();

	private:
		PXCSenseManager *pp;
		PXCCaptureManager *cm;
	};
}