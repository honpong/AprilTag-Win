//
//  sensor_clock.h
//  RC3DK
//
//  Created by Eagle Jones on 3/23/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__sensor_clock__
#define __RC3DK__sensor_clock__

#include <chrono>

class sensor_clock
{
public:
    typedef std::chrono::nanoseconds                        duration;
    typedef duration::rep                                   rep;
    typedef duration::period                                period;
    typedef std::chrono::time_point<sensor_clock, duration> time_point;
    static constexpr const bool is_steady = true;
    
    static time_point now() noexcept;
};

#endif /* defined(__RC3DK__sensor_clock__) */
