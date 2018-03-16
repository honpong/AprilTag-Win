/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2016-2018 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include "tpose.h"
#include "rc_tracker.h"
#include "stream_object.h"
#include "bstream.h"

class replay
{
private:
    std::unique_ptr<host_stream> stream; /// provides packet header and data.
    std::string tracking_stat;
    bool start_paused{ false };
    bool is_started{ false }; /// keep track of API call order of rc_startTracker, rc_loadMap
    double reference_path_length{ NAN };
    double reference_length{ NAN };
    std::unique_ptr<tpose_sequence> reference_seq;
    std::thread replay_thread;
    bool find_reference_in_filename(const std::string &filename);
    bool load_reference_from_pose_file(const std::string &filename);
    bool load_internal_calibration(const std::string &filename);
    void request(uint8_t type) { stream->put_host_packet(packet_command_alloc(type)); }
    void request(uint8_t type, const void *load, size_t load_size) {
        stream->put_host_packet(packet_control_alloc(type, (const char *)load, load_size));
    }
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    replay(host_stream *host_stream_, bool start_paused = false);
    void enable_realtime() { request(packet_enable_realtime); }
    void enable_async() { request(packet_enable_async); }
    void enable_qvga() { request(packet_enable_qvga); }
    void disable_depth() { request(packet_enable_no_depth); }
    void enable_fast_path() { request(packet_enable_fast_path); }
    void enable_odometry() { request(packet_enable_odometry); }
    void enable_qres(uint8_t qres_) { request(packet_enable_qres, &qres_, sizeof(uint8_t)); }
    void zero_biases() { request(packet_enable_zero_biases); }
    void delay_start(uint64_t delay_us) { request(packet_delay_start, &delay_us, sizeof(uint64_t)); }
    void set_message_level(rc_MessageLevel level) {
        uint8_t msg_level = level;
        request(packet_enable_mesg_level, (const char*)&msg_level, sizeof(uint8_t));
    }
    void set_replay_output_mode(uint8_t mode) { request(packet_enable_output_mode, &mode, sizeof(uint8_t)); }

    bool init() { return stream->init_stream(); }
    std::string calibration_file;
    /// location of vocabulary file is optional when running on host system
    void start_mapping(bool relocalize, uint8_t save_map = false);
    bool save_map(const char *filename);
    bool load_map(const char *filename);
    bool load_calibration(const char *filename);
    bool save_calibration(const char *filename);
    bool set_calibration_from_filename(const char *filename);
    void set_progress_callback(std::function<void(float)> progress_callback_) { stream->progress_callback = progress_callback_; }
    void set_data_callback(std::function<void(const replay_output *, const rc_Data *)> data_callback) {
        stream->host_data_callback = data_callback;
    }
    void set_track_stat(std::string stat) { tracking_stat = stat; }
    std::string get_track_stat();
    void start();
    void start_async();
    void stop() { request(packet_command_stop); stream->wait_device_packet({ packet_command_stop }); };
    void reset() { request(packet_command_reset); }
    void toggle_pause() { request(packet_command_toggle_pause); }
    void pause() {
        if (!start_paused) {
            request(packet_command_toggle_pause);
            start_paused = true;
        }
    }
    void set_pause(uint64_t timestamp) { request(packet_command_next_pause, (const char*)&timestamp, sizeof(timestamp)); }
    void step() { request(packet_command_step); }
    void end() {
        request(packet_command_end);
        if (replay_thread.joinable()) replay_thread.join();
    }
    void set_stage() { request(packet_set_stage); }
    rc_Extrinsics get_camera_extrinsics(uint8_t camera_id);
    uint64_t get_bytes_dispatched() { return stream ? stream->get_bytes_dispatched() : 0; }
    uint64_t get_packets_dispatched() { return  stream ? stream->get_packets_dispatched() : 0; }
    void set_relative_pose(const sensor_clock::time_point & timestamp, const tpose & pose);
    bool get_reference_pose(const sensor_clock::time_point & timestamp, tpose & pose_out);
    double get_reference_path_length() { return reference_path_length; }
    double get_reference_length() { return reference_length; }
    bool set_reference_from_filename(const char *filename);
    const tpose_sequence& get_reference_poses() const { return *reference_seq; }
    ~replay();
};
