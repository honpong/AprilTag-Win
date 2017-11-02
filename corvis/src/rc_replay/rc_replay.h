#pragma once
//  Copyright (c) 2015 RealityCap. All rights reserved.
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include "rc_tracker.h"

namespace rc {

class replay
{
private:
    std::ifstream file;
    bool depth {true};
    bool qvga {false};
    double length_m {0}, reference_length_m {NAN};
    bool find_reference_in_filename(const std::string &filename);
    bool set_calibration_from_filename(const std::string &fn);
    virtual bool sleep_until(uint64_t time_us) { return true; }
    bool fast_path { false };

public:
    rc_Tracker *tracker;
    replay() {
        tracker = rc_create();
        rc_setMessageCallback(tracker, [](void *handle, rc_MessageLevel message_level, const char * message, size_t len) {
                std::cerr << message;
            }, nullptr, rc_MESSAGE_ERROR);
    }
    ~replay() { rc_destroy(tracker); tracker = nullptr; }
    void reset() { rc_reset(tracker, 0); }
    bool open(const char *name);
    bool run();
    void enable_fast_path() { fast_path = true; }
    void disable_depth() { depth = false; }
    void enable_qvga() { qvga = true; }
    double get_length() { return length_m; }
    double get_reference_length() { return reference_length_m; }
    const char * get_version() { return rc_version(); }
    void enable_pose_output();
    void enable_tum_output();
    void enable_status_output();
    void start_mapping(bool relocalize);
};

};
