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
#include "packet.h"
#include "tpose.h"
#include "rc_tracker.h"
#include "sensor_fusion.h"

class replay
{
private:
    std::ifstream file;
    std::ifstream::pos_type size;
    std::atomic<uint64_t> packets_dispatched{0};
    std::atomic<uint64_t> bytes_dispatched{0};
    std::mutex lengths_mutex;
    double path_length{0}; double reference_path_length{NAN};
    double length{0}; double reference_length{NAN};
    std::unique_ptr<tpose_sequence> reference_seq;
    std::atomic<bool> is_running{false};
    std::atomic<bool> is_paused{false};
    std::atomic<bool> is_stepping{false};
    std::atomic<uint64_t> next_pause{0};
    bool is_realtime = false;
    std::function<void (rc_Tracker *, const rc_Data *)> data_callback;
    std::function<void (float)> progress_callback;
    bool qvga {false};
    bool use_depth {true};
    bool accel_decimate {false};
    bool gyro_decimate {false};
    bool image_decimate {false};
    std::chrono::microseconds accel_interval {10000};
    std::chrono::microseconds gyro_interval {10000};
    std::chrono::microseconds image_interval {33333};
    sensor_clock::time_point last_accel, last_gyro, last_image;
    bool find_reference_in_filename(const string &filename);
    bool load_reference_from_pose_file(const string &filename);
    bool load_map(std::string filename);

public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    rc_Tracker * tracker;
    replay(bool start_paused=false);
    ~replay() { rc_destroy(tracker); }
    bool open(const char *filename);
    std::string calibration_file;
    bool load_calibration(std::string filename);
    bool save_calibration(std::string filename);
    bool set_calibration_from_filename(const char *filename);
    void setup_filter();
    void set_progress_callback(std::function<void (float)> progress_callback) { this->progress_callback = progress_callback; }
    void set_data_callback(std::function<void (rc_Tracker *, const rc_Data *)> data_callback) { this->data_callback = data_callback; }
    void enable_realtime() { is_realtime = true; }
    void enable_qvga() { qvga = true; }
    void disable_depth() { use_depth = false; }
    void decimate_accel(std::chrono::microseconds interval) { accel_decimate = true; accel_interval = interval; }
    void decimate_gyro(std::chrono::microseconds interval) { gyro_decimate = true; gyro_interval = interval; }
    void decimate_images(std::chrono::microseconds interval) { image_decimate = true; image_interval = interval; }
    void start(string map_filename = string());
    void stop();
    void toggle_pause() { is_paused = !is_paused; }
    void set_pause(uint64_t timestamp) { next_pause = timestamp; }
    void step() { is_paused = is_stepping = true; }
    uint64_t get_bytes_dispatched() { return bytes_dispatched; }
    uint64_t get_packets_dispatched() { return packets_dispatched; }
    double get_path_length() { return path_length; }
    double get_length() { return length; }
    bool get_reference_pose(const sensor_clock::time_point & timestamp, tpose & pose_out);
    double get_reference_path_length() { return reference_path_length; }
    double get_reference_length() { return reference_length; }
    bool set_reference_from_filename(const string &filename);
    void zero_biases();
    void start_mapping() { rc_startMapping(tracker); }
    void save_map(string filename);
};

#endif /* defined(__RC3DK__replay__) */
