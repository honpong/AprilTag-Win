
#include "stdafx.h"
#include "atlbase.h"
#include <string>

#pragma once

struct AccelSample
{
	float x;
	float y;
	float z;
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

