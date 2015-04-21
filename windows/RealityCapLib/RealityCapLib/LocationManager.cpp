#include "pch.h"
#include "LocationManager.h"
#include "Debug.h"

using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Platform;
using namespace RealityCap;
using namespace Windows::Devices::Geolocation;
using namespace concurrency;

LocationManager::LocationManager()
{
}

//LocationManager ^ RealityCap::LocationManager::GetSharedInstance()
//{
//	static LocationManager^ _sharedInstance;
//	if (_sharedInstance == nullptr)
//	{
//		_sharedInstance = ref new LocationManager();
//	}
//	return _sharedInstance;
//}

void RealityCap::LocationManager::GetLocationAndCache()
{
	geolocator = ref new Geolocator();
	geolocator->DesiredAccuracyInMeters = static_cast<unsigned int>(100);

	task<Geoposition^> geopositionTask(geolocator->GetGeopositionAsync(), geopositionTaskTokenSource.get_token());
	geopositionTask.then([this](task<Geoposition^> getPosTask)
	{
		try
		{
			// Get will throw an exception if the task was canceled or failed with an error
			_cachedPosition = getPosTask.get();			
			if (cachedPosition) Debug::Log(L"geolocation lat:%3.2f\tlon:%3.2f", cachedPosition->Coordinate->Point->Position.Latitude, cachedPosition->Coordinate->Point->Position.Longitude);
		}
		catch (AccessDeniedException^)
		{
			// TODO: handle error
			Debug::Log(L"LocationManager: AccessDeniedException");
		}
		catch (task_canceled&)
		{
			// TODO: handle error
			Debug::Log(L"LocationManager: task_canceled");
		}
		catch (Exception^ ex)
		{
#if (WINAPI_FAMILY == WINAPI_FAMILY_PC_APP)
			// If there are no location sensors GetGeopositionAsync() will timeout -- that is acceptable.
			if (ex->HResult == HRESULT_FROM_WIN32(WAIT_TIMEOUT))
			{
				// TODO: handle error
			}
			else
#endif
			{
				// TODO: handle error
				Debug::Log(L"LocationManager: Exception");
			}
		}
	});
}


