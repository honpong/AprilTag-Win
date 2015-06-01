#pragma once

#include "stdafx.h"
#using <Windows.winmd>
#using <Platform.winmd>

namespace RealityCap
{
	ref class LocationManager sealed
	{
	private:
		Windows::Devices::Geolocation::Geolocator^ geolocator;
		concurrency::cancellation_token_source geopositionTaskTokenSource;
		Windows::Devices::Geolocation::Geoposition^ _cachedPosition;

	public:
		LocationManager();
		//static LocationManager^ GetSharedInstance();
		void GetLocationAndCache();
		property Windows::Devices::Geolocation::Geoposition^ cachedPosition
		{
			Windows::Devices::Geolocation::Geoposition^ get() { return _cachedPosition; }
		}
	};
}


