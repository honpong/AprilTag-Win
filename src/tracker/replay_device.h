#pragma once

#include <iostream>
#include <atomic>
#include <functional>
#include "tpose.h"
#include "rc_tracker.h"
#include "stream_object.h"

class replay_device
{
private:
    std::unique_ptr <rc_Tracker, decltype(&rc_destroy)> tracker{ nullptr, &rc_destroy };
    device_stream *stream{nullptr}; /// provides packet header and data.
    // configuration members
    std::atomic<bool> should_reset{false}, is_running{false}, is_paused{false}, is_stepping{false};
    std::atomic<uint64_t> next_pause{0};
    uint64_t delay_start{0};
    rc_MessageLevel message_level = rc_MESSAGE_WARN;
    bool is_realtime{ false }, qvga{ false }, use_depth{ true };
    bool to_zero_biases{ false }, use_odometry{ false }, stereo_configured{ false };
    uint8_t qres{ 0 };
    std::chrono::microseconds realtime_offset{ 0 };
    rc_TrackerQueueStrategy queue_strategy{ rc_QUEUE_MINIMIZE_DROPS };
    rc_TrackerRunFlags run_flags{ rc_RUN_SYNCHRONOUS };
    bool strategy_override{ false };
    void setup_filter();
    void process_data(rc_packet_t &phandle);
    void process_control(const packet_control_t *packet);
    void set_stage();
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    replay_device() {};
    bool init(device_stream *_stream_object, std::unique_ptr<rc_Tracker, decltype(&rc_destroy)> tracker);
    void start();
    void stop() { is_running = false; }
    void zero_bias();
    ~replay_device() {}
};
