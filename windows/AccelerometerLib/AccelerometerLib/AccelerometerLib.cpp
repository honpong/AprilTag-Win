// AccelerometerLib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "AccelerometerLib.h"
#include "AccelerometerManager.h"

bool SetChangeSensitivity(double sensitivity)
{
	AccelerometerManager ameterMan;
	HRESULT hr = ameterMan.SetChangeSensitivity(sensitivity);
	return SUCCEEDED(hr);
}

bool SetReportingInterval(ULONG milliseconds)
{
	AccelerometerManager ameterMan;
	HRESULT hr = ameterMan.SetReportInterval(milliseconds);
	return SUCCEEDED(hr);
}


