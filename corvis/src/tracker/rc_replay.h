#pragma once
//  Copyright (c) 2015 RealityCap. All rights reserved.
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include "rc_intel_interface.h"

namespace rc {

class replay
{
private:
    std::ifstream file;
    bool depth {true};
    double length_m {0}, reference_length_m {NAN};
    bool find_reference_in_filename(const std::string &filename);
    bool set_calibration_from_filename(const std::string &fn);
    virtual bool sleep_until(uint64_t time_us) { return true; }

public:
    rc_Tracker *tracker;
    replay() { tracker = rc_create(); }
    ~replay() { rc_destroy(tracker); tracker = nullptr; }
    void reset() { rc_reset(tracker, 0, rc_Pose{1,0,0,0, 0,1,0,0, 0,0,1,0}); }
    bool open(const char *name);
    bool run();
    void disable_depth() { depth = false; }
    double get_length() { return length_m; }
    double get_reference_length() { return reference_length_m; }
    void enable_pose_output();
    void enable_status_output();
    void enable_log_output(bool stream, rc_Timestamp period_us);
};

};
