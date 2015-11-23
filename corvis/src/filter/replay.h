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
#include "../cor/packet.h"
#include "tpose.h"
#include "sensor_fusion.h"

class replay
{
private:
    std::ifstream file;
    std::ifstream::pos_type size;
    std::atomic<uint64_t> packets_dispatched{0};
    std::atomic<uint64_t> bytes_dispatched{0};
    std::atomic<double> path_length{0}; double reference_path_length{NAN};
    std::atomic<double> length{0}; double reference_length{NAN};
    std::unique_ptr<tpose_sequence> reference_seq;
    std::atomic<bool> is_running{false};
    std::atomic<bool> is_paused{false};
    std::atomic<bool> is_stepping{false};
    bool is_realtime = false;
    std::function<void (const filter *, camera_data &&)> camera_callback;
    std::function<void (float)> progress_callback;
    bool qvga {false};
    bool depth {true};
    image_gray8 parse_gray8(int width, int height, int stride, uint8_t *data, uint64_t time_us, uint64_t exposure_time_us, std::unique_ptr<void, void(*)(void *)> handle);
    bool find_reference_in_filename(const string &filename);
    bool load_reference_from_pose_file(const string &filename);

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    sensor_fusion fusion;
    replay(bool start_paused=false) : is_paused(start_paused), fusion(fusion_queue::latency_strategy::ELIMINATE_DROPS) {}
    bool open(const char *filename);
    std::string calibration_file;
    bool set_calibration_from_filename(const char *filename);
    void setup_filter();
    void set_progress_callback(std::function<void (float)> progress_callback) { this->progress_callback = progress_callback; }
    void set_camera_callback(std::function<void (const filter *, camera_data)> camera_callback) { this->camera_callback = camera_callback; }
    void enable_realtime() { is_realtime = true; }
    void enable_qvga() { qvga = true; }
    void disable_depth() { depth = false; }
    void enable_intel() { fusion.queue->strategy = fusion_queue::latency_strategy::IMAGE_TRIGGER; fusion.sfm.ignore_lateness = true; }
    void start();
    void stop();
    void toggle_pause() { is_paused = !is_paused; }
    void step() { is_paused = is_stepping = true; }
    uint64_t get_bytes_dispatched() { return bytes_dispatched; }
    uint64_t get_packets_dispatched() { return packets_dispatched; }
    double get_path_length() { return path_length; }
    double get_length() { return length; }
    double get_reference_path_length() { return reference_path_length; }
    double get_reference_length() { return reference_length; }
    bool set_reference_from_filename(const string &filename);
    device_parameters get_device_parameters() const { return fusion.get_device(); }
    std::string get_timing_stats() { return fusion.get_timing_stats(); }
};

#endif /* defined(__RC3DK__replay__) */
