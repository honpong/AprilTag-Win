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
    typedef std::chrono::microseconds                       duration;
    typedef duration::rep                                   rep;
    typedef duration::period                                period;
    typedef std::chrono::time_point<sensor_clock, duration> time_point;
    static constexpr const bool is_steady = true;

    static time_point now() noexcept;
    static uint64_t tp_to_micros(const time_point t) { return std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count(); }
    static time_point micros_to_tp(const uint64_t m) { return time_point(std::chrono::microseconds(m)); }

    static time_point s_ns_to_tp(uint64_t s, uint64_t ns) { return time_point(std::chrono::duration_cast<duration>(std::chrono::seconds(s) + std::chrono::nanoseconds(ns)));};
    static time_point ns100_to_tp(uint64_t ns100) { return time_point(std::chrono::duration_cast<duration>(std::chrono::duration<uint64_t, std::ratio<1, 10000000>>(ns100))); }
    static time_point s_to_tp(double s) { return time_point(std::chrono::duration_cast<duration>(std::chrono::duration<double, std::chrono::seconds::period>(s))); }
};

#endif /* defined(__RC3DK__sensor_clock__) */
