#pragma once

#include "stdafx.h"

#ifdef ACCELEROMETERLIB_EXPORTS
#define ACCELEROMETERLIB_API __declspec(dllexport) 
#else
#define ACCELEROMETERLIB_API __declspec(dllimport) 
#endif

ACCELEROMETERLIB_API bool SetChangeSensitivity(double sensitivity);
ACCELEROMETERLIB_API bool SetReportingInterval(ULONG milliseconds);