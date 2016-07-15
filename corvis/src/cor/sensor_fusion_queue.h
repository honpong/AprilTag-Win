//
//  sensor_fusion_queue.h
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#ifndef __sensor_fusion_queue__
#define __sensor_fusion_queue__

#include <array>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "sensor_data.h"
#include "platform/sensor_clock.h"
#include "../numerics/vec4.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <deque>

struct sensor_data;
/*
 Intention is that this fusion queue outputs a causal stream of data:
 A measurement with timestamp t is *complete* at that time, and all measurements are delivered in order relative to these timestamps
 For cameras or depth cameras, t should be the middle of the integration period
 */
class fusion_queue
{
public:
    enum class latency_strategy
    {
        FIFO, //pass data through the queue without any modification
        MINIMIZE_LATENCY, //minimize latency
        DYNAMIC_LATENCY, //start with high latency and dynamically adjust as sensor data arrives
        ELIMINATE_DROPS //we always wait until the data in the other queues is ready
    };

    fusion_queue(const std::function<void(sensor_data &&)> &receive_func,
                 latency_strategy s,
                 sensor_clock::duration initial_latency);
    ~fusion_queue();
    
    void reset();

    void start_async();
    void start_sync();
    void start_singlethreaded();
    void start_buffering();
    void stop();

    void require_sensor(rc_SensorType type, rc_Sensor id);

    void receive_sensor_data(sensor_data &&);
    void dispatch_sync(std::function<void()> fn);
    void dispatch_async(std::function<void()> fn);

    std::string get_stats();

    latency_strategy strategy;

    uint64_t total_in{0};
    uint64_t total_out{0};

private:
    int cameras{0};
    int depths{0};
    int accelerometers{0};
    int gyroscopes{0};
    void clear();
    void stop_async();
    void stop_immediately();
    void wait_until_finished();
    void runloop();
    bool run_control();
    bool ok_to_dispatch();
    bool dispatch_next(std::unique_lock<std::mutex> &lock, bool force);
    void dispatch_singlethread(bool force);
    void dispatch_buffer();
    void push_queue(uint64_t global_id, sensor_data &&);
    sensor_data pop_queue();


    bool all_have_data();

    std::mutex data_lock;
    std::mutex control_lock;
    std::condition_variable cond;
    std::thread thread;
    
    std::function<void(sensor_data &&)> data_receiver;
    
    std::unordered_map<uint64_t, uint64_t> queue_count;
    std::unordered_map<uint64_t, uint64_t> dispatch_count;
    std::unordered_map<uint64_t, sensor_clock::time_point> latest_seen;
    std::vector<uint64_t> required_sensors;
    std::deque<sensor_data> queue;

    std::function<void()> control_func;
    bool active;
    bool singlethreaded;
    
    sensor_clock::time_point last_dispatched;
    
    sensor_clock::duration latency;
};

#endif /* defined(__sensor_fusion_queue__) */
