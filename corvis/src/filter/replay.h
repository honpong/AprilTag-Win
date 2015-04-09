//
//  replay.h
//
//  Created by Eagle Jones on 4/8/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__replay__
#define __RC3DK__replay__

#include <iostream>
#include <fstream>
#include "filter_setup.h"
#include "sensor_fusion_queue.h"

class replay
{
private:
    std::ifstream file;
    std::ifstream::pos_type size;
    int packets_dispatched = 0;
    uint64_t bytes_dispatched = 0;
    bool is_running = true;
    std::unique_ptr<filter_setup> cor_setup;
    std::unique_ptr<fusion_queue> queue;
    uint64_t first_timestamp;

public:
    bool open(const char *name);
    void set_device(const char *name);
    void setup_filter();    
    bool configure_all(const char *filename, const char *devicename);
    void runloop();
    
};

#endif /* defined(__RC3DK__replay__) */
