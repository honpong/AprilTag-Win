
#include "pch.h"
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

    // Register the window class and call methods for instantiating drawing resources
    HRESULT Initialize();

	AccelSample GetSample();
	HRESULT SetChangeSensitivity(double sensitivity); // in m/s^2

private:

	// Sensor interface pointers
	ISensorManager* pSensorManager; 
	ISensorCollection* pSensorColl;
	ISensor* pSensor; 
};

