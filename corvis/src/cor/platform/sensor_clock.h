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
#ifdef WIN32
    typedef std::chrono::duration<long long, std::ratio<1, 10000000>> duration;
#else
    typedef std::chrono::nanoseconds                        duration;
#endif
    typedef duration::rep                                   rep;
    typedef duration::period                                period;
    typedef std::chrono::time_point<sensor_clock, duration> time_point;
    static constexpr const bool is_steady = true;
    
    static time_point now() noexcept;
    static uint64_t tp_to_micros(const time_point t) { return std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count(); }
    static time_point micros_to_tp(const uint64_t m) { return time_point(std::chrono::microseconds(m)); }

};

#endif /* defined(__RC3DK__sensor_clock__) */
