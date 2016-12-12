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
#include "vec4.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <deque>
#include "stdev.h"
#ifdef DEBUG
#include "histogram.h"
#endif

class sensor_stats
{
public:
    sensor_stats(sensor_clock::duration maximum_latency) :
        max_latency(maximum_latency) {};

    sensor_clock::duration max_latency;
    sensor_clock::time_point last_in{};
    uint64_t in{0};
    uint64_t in_queue{0};
    uint64_t out{0};
    uint64_t out_of_order{0};
    stdev<1> late{};
    stdev<1> period{};
    stdev<1> latency{};

#ifdef DEBUG
    histogram hist{200};
#endif

    bool expected(const sensor_clock::time_point & time) {
        if(!in) return false;
        if(last_in > time) return false;

        return !period.count || time - last_in > std::chrono::microseconds(uint64_t(std::max(f_t(0), period.min[0])));
    }

    bool late_dynamic_latency(const sensor_clock::time_point & now) {
        if(last_in > now) return false;

        return period.valid() && latency.valid() && now - last_in > std::chrono::microseconds(uint64_t(period.mean[0] + latency.mean[0] + latency.stdev_[0]*3));
    }

    bool late_fixed_latency(const sensor_clock::time_point & now) {
        if(last_in > now) return false;

        return period.valid() && now - last_in > std::chrono::microseconds(uint64_t(period.mean[0])) + max_latency;
    }

    void receive(const sensor_clock::time_point & now, const sensor_clock::time_point & timestamp) {
        if(in > 0 && timestamp >= last_in) {
            sensor_clock::duration delta = timestamp - last_in;
            period.data(v<1>{(f_t)delta.count()});
            latency.data(v<1>{(f_t)(now - timestamp).count()});
#ifdef DEBUG
            hist.data(delta.count());
#endif
        }
        in++;
        if(timestamp >= last_in)
            last_in = timestamp;
    }

    void push() { in_queue++; }
    void dispatch() { out++; in_queue--; }
    void drop_out_of_order() { out_of_order++; }
    void drop_buffered() { /*unbuffered++;*/ }
    void drop_late(const sensor_clock::time_point & now) {
        late.data(v<1>{(f_t)((now - last_in).count())});
    }

    std::string to_string() const {
        std::ostringstream os;
        os << in << " in, " << out << " out, " << late.count << " late " << out_of_order << " out of order\n";
        if (period.count)  os << "\tperiod(us):  " << period  << "\n";
        if (latency.count) os << "\tlatency(us): " << latency << "\n";
        if (late.count)    os << "\tlate(us):    " << late    << "\n";
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
        MINIMIZE_LATENCY, //minimize latency
        DYNAMIC_LATENCY, //estimate relative latency and use it to determine drops
        ELIMINATE_DROPS //we always wait until the data in the other queues is ready
    };

    fusion_queue(std::function<void(sensor_data &&)> receive_func,
                 latency_strategy s,
                 sensor_clock::duration max_latency);
    ~fusion_queue();
    
    void reset();

    void start(bool threaded);
    void start_buffering(sensor_clock::duration buffer_time);

    void stop();

    void require_sensor(rc_SensorType type, rc_Sensor id, sensor_clock::duration max_latency);

    void receive_sensor_data(sensor_data &&);
    void dispatch_async(std::function<void()> fn);

    std::string get_stats();

    void dispatch_buffered(std::function<void(sensor_data &)> receive_func);

    latency_strategy strategy;

    uint64_t total_in{0};
    uint64_t total_out{0};
    sensor_clock::time_point newest_received{};

private:
    stdev<1> queue_latency{};
    void clear();
    void stop_async();
    void stop_immediately();
    void wait_until_finished();
    void runloop();
    bool run_control();
    bool ok_to_dispatch();
    bool dispatch_next(std::unique_lock<std::mutex> &lock, bool force);
    void dispatch_singlethread(bool force);
    void push_queue(uint64_t global_id, sensor_data &&);
    sensor_data pop_queue();
    sensor_clock::time_point next_timestamp();

    bool all_have_data();

    std::mutex data_mutex;
    std::mutex control_mutex;
    std::condition_variable cond;
    std::thread thread;
    
    std::function<void(sensor_data &&)> data_receiver;
    std::function<void(sensor_data &, bool)> fast_data_receiver;
    
    std::unordered_map<uint64_t, sensor_stats> stats;
    std::vector<uint64_t> required_sensors;
    std::deque<sensor_data> queue;

    std::function<void()> control_func;
    bool active;
    bool singlethreaded;
    
    sensor_clock::time_point last_dispatched;
    
    sensor_clock::duration max_latency;
    sensor_clock::duration buffer_time{};
    bool buffering{false};
};

#endif /* defined(__sensor_fusion_queue__) */
