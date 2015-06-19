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
#include <atomic>
#include <memory>
#include <functional>
#include "filter_setup.h"
#include "../cor/packet.h"
#include "../cor/sensor_fusion_queue.h"

class replay
{
private:
    std::ifstream file;
    std::ifstream::pos_type size;
    std::atomic<uint64_t> packets_dispatched{0};
    std::atomic<uint64_t> bytes_dispatched{0};
    std::atomic<float> path_length{0};
    std::atomic<float> length{0};
    std::atomic<bool> is_running{false};
    std::atomic<bool> is_paused{false};
    std::atomic<bool> is_stepping{false};
    bool is_realtime = false;
    std::unique_ptr<filter_setup> cor_setup;
    std::unique_ptr<fusion_queue> queue;
    std::function<void (const filter *, camera_data &&)> camera_callback;
    std::function<void (float)> progress_callback;

public:
    replay(bool start_paused=false) : is_paused(start_paused) {}
    bool open(const char *name);
    bool set_device(const char *name);
    bool set_calibration_from_filename(const char *filename);
    void setup_filter();
    bool configure_all(const char *filename, const char *devicename, bool realtime=false, std::function<void (float)> progress_callback=nullptr, std::function<void (const filter *, camera_data)> camera_callback=nullptr);
    void start();
    void stop();
    void toggle_pause() { is_paused = !is_paused; }
    void step() { is_paused = is_stepping = true; }
    uint64_t get_bytes_dispatched() { return bytes_dispatched; }
    uint64_t get_packets_dispatched() { return packets_dispatched; }
    float get_path_length() { return path_length; }
    float get_length() { return length; }
};

#endif /* defined(__RC3DK__replay__) */
