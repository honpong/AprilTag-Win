#pragma once

#include "stdafx.h"
#include "atlbase.h"

struct AccelSample
{
	double x;
	double y;
	double z;
	SYSTEMTIME timestamp;
};

class AccelerometerManager
{
public:
	AccelerometerManager(void);
	~AccelerometerManager(void);

    HRESULT Initialize();
	AccelSample GetSample();
	HRESULT SetChangeSensitivity(double sensitivity); // in m/s^2
	HRESULT SetReportInterval(ULONG milliseconds); 

private:
	CComPtr<ISensor> pSensor; 
	HRESULT SetProperties(IPortableDeviceValues* pInValues);
};

