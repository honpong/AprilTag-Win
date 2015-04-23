// AccelerometerLib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "AccelerometerLib.h"
#include "AccelerometerManager.h"

bool SetAccelerometerSensitivity(double sensitivity)
{
	// use shared instance because object must stay alive for settings to stick
	AccelerometerManager* ameterMan = AccelerometerManager::GetSharedInstance(); 
	HRESULT hr;
	hr = ameterMan->Initialize();
	if (!SUCCEEDED(hr)) return false;
	hr = ameterMan->SetChangeSensitivity(sensitivity);
	return SUCCEEDED(hr);
}



