#pragma once

#include "stdafx.h"

#ifdef ACCELEROMETERLIB_EXPORTS
#define ACCELEROMETERLIB_API __declspec(dllexport) 
#else
#define ACCELEROMETERLIB_API __declspec(dllimport) 
#endif

ACCELEROMETERLIB_API bool SetAccelerometerSensitivity(double sensitivity);