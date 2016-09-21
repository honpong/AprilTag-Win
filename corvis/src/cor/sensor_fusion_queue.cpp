//
//  sensor_fusion_queue.cpp
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "../filter/rc_tracker.h"
#include "sensor_fusion_queue.h"
#include <cassert>
#include <memory>
#include <algorithm>

#define MAX_SENSORS 64
inline std::ostream & operator <<(std::ostream & s, const std::vector<sensor_data> &v) {
    for(int i = 0; i < v.size(); i++)
        s << sensor_clock::tp_to_micros(v[i].timestamp) << ":" << v[i].type << " ";
    return s;
}

static bool compare_sensor_data(const sensor_data &d1, const sensor_data &d2) {
    if(d1.timestamp == d2.timestamp)
        return d1.type > d2.type;
    return d1.timestamp > d2.timestamp;
}

fusion_queue::fusion_queue(const std::function<void(sensor_data &&)> &data_func,
                           latency_strategy s,
                           sensor_clock::duration maximum_latency):
                strategy(s),
                data_receiver(data_func),
                control_func(nullptr),
                active(false),
                singlethreaded(false),
                max_latency(maximum_latency)
{
}

void fusion_queue::require_sensor(rc_SensorType type, rc_Sensor id, sensor_clock::duration max_latency)
{
    uint64_t global_id = id + type*MAX_SENSORS;
    required_sensors.push_back(global_id);
    auto s = stats.emplace(global_id, sensor_stats{max_latency});
}

void fusion_queue::reset()
{
    stop_immediately();
    wait_until_finished();
    thread = std::thread();
    clear();

    control_func = nullptr;
    active = false;
    last_dispatched = sensor_clock::time_point();
}

fusion_queue::~fusion_queue()
{
    stop_immediately();
    wait_until_finished();
}

std::string id_string(uint64_t global_id)
{
    int type = global_id / MAX_SENSORS;
    std::string type_string = "UNKNOWN";
    switch(type) {
        case rc_SENSOR_TYPE_IMAGE: type_string = "Camera"; break;
        case rc_SENSOR_TYPE_DEPTH: type_string = "Depth"; break;
        case rc_SENSOR_TYPE_ACCELEROMETER: type_string = "Accel"; break;
        case rc_SENSOR_TYPE_GYROSCOPE: type_string = "Gyro"; break;
    }
    int id = global_id - type * MAX_SENSORS;
    return type_string + std::to_string(id);
}

std::string fusion_queue::get_stats()
{
    std::ostringstream statstr;

    std::vector<uint64_t> keys;
    for(auto kv : stats) {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    for(auto k : keys) {
        statstr << id_string(k) << "\t" + stats.find(k)->second.to_string() << "\n";
    }

    statstr << "Queue latency: " << queue_latency << "\n";
    
    return statstr.str();
}

void fusion_queue::receive_sensor_data(sensor_data && x)
{
    uint64_t id = x.id + MAX_SENSORS*x.type;
    push_queue(id, std::move(x));
    if(singlethreaded || buffering) dispatch_singlethread(false);
    else cond.notify_one();
}

void fusion_queue::dispatch_async(std::function<void()> fn)
{
    std::unique_lock<std::mutex> lock(control_lock);
    control_func = fn;
    lock.unlock();
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::start_buffering(sensor_clock::duration _buffer_time)
{
    std::unique_lock<std::mutex> lock(control_lock);
    active = true;
    buffer_time = _buffer_time;
    buffering = true;
    lock.unlock();
}

void fusion_queue::start(bool threaded)
{
    singlethreaded = !threaded;
    if(threaded) {
        if(!thread.joinable()) {
            std::unique_lock<std::mutex> lock(control_lock);
            thread = std::thread(&fusion_queue::runloop, this);
            while(!active) {
                cond.wait(lock);
            }
        }
    }
    else {
        active = true;
        buffer_time = {};
        dispatch_singlethread(false); //dispatch any waiting data in case we were buffering
   }
}

void fusion_queue::stop_immediately()
{
    std::unique_lock<std::mutex> lock(control_lock);
    active = false;
    lock.unlock();
    cond.notify_one();
}

void fusion_queue::stop_async()
{
    if(singlethreaded)
    {
        //flush any waiting data
        dispatch_singlethread(true);
#ifdef DEBUG
        std::cerr << get_stats();
        //std::cerr << gyro_queue->hist;
#endif
    }
    stop_immediately();
}

void fusion_queue::stop()
{
    stop_async();
    if(!singlethreaded) wait_until_finished();
}

void fusion_queue::wait_until_finished()
{
    if(thread.joinable()) thread.join();
}

bool fusion_queue::run_control()
{
    if(!control_func) return false;
    control_func();
    control_func = nullptr;
    return true;
}

void fusion_queue::clear()
{
    std::lock_guard<std::mutex> data_guard(data_lock);
    queue.clear();
    stats.clear();
    total_in = 0;
    total_out = 0;
}

void fusion_queue::runloop()
{
    std::unique_lock<std::mutex> lock(control_lock);
    active = true;
    buffering = false;
    buffer_time = {};
    //If we were launched synchronously, wake up the launcher
    lock.unlock();
    cond.notify_one();
    lock.lock();
    while(active)
    {
        while(active && !run_control() && !dispatch_next(lock, false))
        {
            cond.wait(lock);
        }
        while(run_control() || dispatch_next(lock, false)); //we need to be greedy, since we only get woken on new data arriving
    }
    //flush any remaining data
    while (dispatch_next(lock, true));
#ifdef DEBUG
    std::cerr << get_stats();
#endif
    lock.unlock();
}

void fusion_queue::push_queue(uint64_t global_id, sensor_data && x)
{
    std::lock_guard<std::mutex> data_guard(data_lock);
    total_in++;
    newest_received = std::max(x.timestamp, newest_received);

    auto s = stats.emplace(global_id, sensor_stats{std::chrono::microseconds(0)});
    s.first->second.receive(newest_received, x.timestamp);

    if(strategy != latency_strategy::FIFO) {
        if (x.timestamp < last_dispatched) {
            s.first->second.drop_late(newest_received);
            return;
        }
        if (x.timestamp < s.first->second.last_in) {
            s.first->second.drop_out_of_order();
            return;
        }
    }

    s.first->second.push();
    queue.push_back(std::move(x));
    if(strategy != latency_strategy::FIFO)
        std::push_heap(queue.begin(), queue.end(), compare_sensor_data);
}

sensor_clock::time_point fusion_queue::next_timestamp()
{
    // the front of the queue has the earliest pushed item (in the
    // case of FIFO) or the max heap item (in all other latency
    // strategies)
    return queue.front().timestamp;
}

sensor_data fusion_queue::pop_queue()
{
    if(strategy == latency_strategy::FIFO) {
        sensor_data data = std::move(queue.front());
        queue.pop_front();
        last_dispatched = data.timestamp;
        return std::move(data);
    }
    else {
        std::pop_heap(queue.begin(), queue.end(), compare_sensor_data);
        sensor_data data = std::move(queue.back());
        queue.pop_back();
        last_dispatched = data.timestamp;
        return std::move(data);
    }
}

bool fusion_queue::ok_to_dispatch()
{
    sensor_clock::time_point next_time = next_timestamp();
    if(newest_received - next_time > max_latency) return true;
    for(const auto & id : required_sensors) {
        const auto stat = stats.find(id);
        if(stat == stats.end()) return false;
        auto & s = stat->second;
        switch(strategy) {

        case latency_strategy::FIFO:
            return true;
            break;

        case latency_strategy::MINIMIZE_LATENCY:
            if(s.expected(next_time) && !s.late_fixed_latency(newest_received))
                return false;
            break;

        case latency_strategy::DYNAMIC_LATENCY:
            if(s.expected(next_time) && !s.late_dynamic_latency(newest_received))
                return false;
            break;

        case latency_strategy::ELIMINATE_DROPS:
            if(s.in_queue == 0)
                return false;
            break;
        }
    }
    return true;
}

bool fusion_queue::dispatch_next(std::unique_lock<std::mutex> &lock, bool force)
{
    std::lock_guard<std::mutex> data_guard(data_lock);

    if(queue.empty()) return false;

    if(buffering) {
        sensor_clock::time_point next_time = next_timestamp();
        while(newest_received - next_time > buffer_time) {
            sensor_data dropped = pop_queue();
            uint64_t id = dropped.id + dropped.type*MAX_SENSORS;
            stats.find(id)->second.drop_buffered();
            next_time = next_timestamp();
        }
        return false;
    }

    if(!force && !ok_to_dispatch()) return false;

    sensor_data data = pop_queue();

    uint64_t id = data.id + data.type*MAX_SENSORS;
    stats.find(id)->second.dispatch();
    queue_latency.data({f_t((newest_received - data.timestamp).count())});

    lock.unlock();
    data_receiver(std::move(data));
    total_out++;
            
    lock.lock();

    return true;
}

void fusion_queue::dispatch_singlethread(bool force)
{
    std::unique_lock<std::mutex> lock(control_lock);
    while(dispatch_next(lock, force)); //always be greedy - could have multiple pieces of data buffered
    lock.unlock();
}

