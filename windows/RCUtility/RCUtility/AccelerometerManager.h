#pragma once

#include "stdafx.h"
#include "atlbase.h"
#include <string>

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

private:
	CComPtr<ISensor> pSensor; 
};

