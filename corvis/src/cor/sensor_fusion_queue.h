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

class sensor_stats
{
public:
    sensor_stats(uint64_t maximum_latency_us) :
        max_latency_us(maximum_latency_us) {};

    uint64_t max_latency_us;
    uint64_t last_in{0};
    uint64_t in{0};
    uint64_t in_queue{0};
    uint64_t out{0};
    uint64_t dropped{0};
    uint64_t period{0};
    stdev<1> var{};
    stdev<1> latency{};
#ifdef DEBUG
    histogram hist{200};
#endif

    bool expected(uint64_t time_us) {
        if(in < 2) return true;

        return time_us > last_in + period - 1000;
    }

    bool late_dynamic_latency(uint64_t now_us) {
        if(in < 2) return false;

        return now_us > last_in + period + latency.mean[0] + latency.stdev_[0]*3;
    }

    bool late_fixed_latency(uint64_t now_us) {
        if(in < 2) return false;

        return now_us > last_in + period + max_latency_us;
    }

    void receive(uint64_t now, uint64_t timestamp) {
        in_queue++;
        if(in > 0 && timestamp >= last_in) {
            uint64_t delta = timestamp - last_in;
            if(period == 0) period = delta;
            var.data(v<1>{(f_t)delta});
            latency.data(v<1>{(f_t)(now - timestamp)});
#ifdef DEBUG
            hist.data(delta);
#endif

            period = period*(1-0.1) + delta*0.1;
        }
        last_in = timestamp;
        in++;
    }

    void dispatch() { out++; in_queue--; }
    void drop() { dropped++; }

    std::string to_string() const {
        std::ostringstream os;
        os << in << " in, " << out << " out, " << dropped << " dropped\t" << period << "us period ";
        os << "(" << "mean " << var.mean[0] << ", stdev " << var.stdev_[0] << ", max " << var.maximum << ")";
        os << "\n\tLatency: " << "mean " << latency.mean[0] << ", stdev " << latency.stdev_[0] << ", max " << latency.maximum;
        return os.str();

    }
};

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
        DYNAMIC_LATENCY, //estimate relative latency and use it to determine drops
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

    void require_sensor(rc_SensorType type, rc_Sensor id, uint64_t max_latency_us);

    void receive_sensor_data(sensor_data &&);
    void dispatch_sync(std::function<void()> fn);
    void dispatch_async(std::function<void()> fn);

    std::string get_stats();

    latency_strategy strategy;

    uint64_t total_in{0};
    uint64_t total_out{0};
    uint64_t newest_received_us{0};

private:
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
    uint64_t next_timestamp();

    bool all_have_data();

    std::mutex data_lock;
    std::mutex control_lock;
    std::condition_variable cond;
    std::thread thread;
    
    std::function<void(sensor_data &&)> data_receiver;
    
    std::unordered_map<uint64_t, sensor_stats> stats;
    std::vector<uint64_t> required_sensors;
    std::deque<sensor_data> queue;

    std::function<void()> control_func;
    bool active;
    bool singlethreaded;
    
    sensor_clock::time_point last_dispatched;
    
    sensor_clock::duration latency;
};

#endif /* defined(__sensor_fusion_queue__) */
