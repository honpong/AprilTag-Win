//
//  sensor_clock.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/23/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "../sensor_clock.h"

sensor_clock::time_point sensor_clock::now() noexcept
{
    //TODO: this just aliases to the steady_clock, but need the actual windows sensor clock
    return time_point(std::chrono::steady_clock::now().time_since_epoch());
}
