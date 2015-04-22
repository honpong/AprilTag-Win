//
//  sensor_clock.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/23/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "../sensor_clock.h"
#include <windows.h>

static double compute_sensor_factor()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return 1000000000. / frequency.QuadPart;
}

sensor_clock::time_point sensor_clock::now() noexcept
{
    static double factor = compute_sensor_factor();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return time_point(duration(static_cast<rep>(counter.QuadPart * factor)));
}
