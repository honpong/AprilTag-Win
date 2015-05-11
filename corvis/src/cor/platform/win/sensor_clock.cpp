//
//  sensor_clock.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/23/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "../sensor_clock.h"
#include <windows.h>

sensor_clock::time_point sensor_clock::now() noexcept
{
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    ULARGE_INTEGER ftl;
    ftl.HighPart = ft.dwHighDateTime;
    ftl.LowPart = ft.dwLowDateTime;
    return sensor_clock::time_point(sensor_clock::duration(ftl.QuadPart));
}
