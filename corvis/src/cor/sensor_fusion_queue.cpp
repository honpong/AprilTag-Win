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

#define MAX_SENSORS 100
inline std::ostream & operator <<(std::ostream & s, const std::vector<sensor_data> &v) {
    for(int i = 0; i < v.size(); i++)
        s << v[i].time_us << ":" << v[i].type << " ";
    return s;
}

bool compare_sensor_data(const sensor_data &d1, const sensor_data &d2) {
    if(d1.time_us == d2.time_us)
        return d1.type > d2.type;
    return d1.time_us > d2.time_us;
}

fusion_queue::fusion_queue(const std::function<void(sensor_data &&)> &data_func,
                           latency_strategy s,
                           sensor_clock::duration max_latency):
                strategy(s),
                data_receiver(data_func),
                control_func(nullptr),
                active(false),
                singlethreaded(false),
                latency(max_latency)
{
}

void fusion_queue::require_sensor(rc_SensorType type, rc_Sensor id)
{
    required_sensors.push_back(id + type*MAX_SENSORS);
}

void fusion_queue::reset()
{
    stop_immediately();
    wait_until_finished();
    thread = std::thread();

    control_func = nullptr;
    active = false;
    last_dispatched = sensor_clock::time_point();
}

fusion_queue::~fusion_queue()
{
    stop_immediately();
    wait_until_finished();
}

std::string fusion_queue::get_stats()
{
    std::string stats;
    
    return stats;
}

void fusion_queue::receive_sensor_data(sensor_data && x)
{
    uint64_t id = x.id + MAX_SENSORS*x.type;
    push_queue(id, std::move(x));
    cond.notify_one();
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::dispatch_sync(std::function<void()> fn)
{
    std::unique_lock<std::mutex> lock(control_lock);
    fn();
    lock.unlock();
}

void fusion_queue::dispatch_async(std::function<void()> fn)
{
    std::unique_lock<std::mutex> lock(control_lock);
    control_func = fn;
    lock.unlock();
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::start_buffering()
{
    std::unique_lock<std::mutex> lock(control_lock);
    active = true;
    lock.unlock();
}

void fusion_queue::start_async()
{
    if(!thread.joinable())
    {
        thread = std::thread(&fusion_queue::runloop, this);
    }
}

void fusion_queue::start_sync()
{
    if(!thread.joinable())
    {
        std::unique_lock<std::mutex> lock(control_lock);
        thread = std::thread(&fusion_queue::runloop, this);
        while(!active)
        {
            cond.wait(lock);
        }
    }
}

void fusion_queue::start_singlethreaded()
{
    singlethreaded = true;
    active = true;
    dispatch_singlethread(false); //dispatch any waiting data in case we were buffering
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

void fusion_queue::runloop()
{
    std::unique_lock<std::mutex> lock(control_lock);
    active = true;
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
    if(latest_seen.find(global_id) != latest_seen.end() && latest_seen[global_id] > sensor_clock::micros_to_tp(x.time_us)) {
        //drop_late++;
        return;
    }

    latest_seen[global_id] = sensor_clock::micros_to_tp(x.time_us);
    queue.push_back(std::move(x));
    if(strategy != latency_strategy::FIFO)
        std::push_heap(queue.begin(), queue.end(), compare_sensor_data);

    if(queue_count.find(global_id) == queue_count.end())
        queue_count[global_id] = 1;
    else
        queue_count[global_id]++;
}

sensor_data fusion_queue::pop_queue()
{
    if(strategy == latency_strategy::FIFO) {
        sensor_data data = std::move(queue.front());
        queue.pop_front();
        last_dispatched = sensor_clock::micros_to_tp(data.time_us);
        return std::move(data);
    }
    else {
        std::pop_heap(queue.begin(), queue.end(), compare_sensor_data);
        sensor_data data = std::move(queue.back());
        queue.pop_back();
        last_dispatched = sensor_clock::micros_to_tp(data.time_us);
        return std::move(data);
    }
}

bool fusion_queue::all_have_data()
{
    if(queue.empty()) return false;
    for(const auto & id : required_sensors) {
        const auto qc = queue_count.find(id);
        if(qc == queue_count.end() || qc->second == 0) {
            return false;
        }
    }

    return true;
}

bool fusion_queue::ok_to_dispatch()
{
    if(strategy == latency_strategy::FIFO)
        return true;
    else if(strategy == latency_strategy::DYNAMIC_LATENCY) {
//        if(last_in != sensor_clock::time_point() && last_in + latency > now)
//            return true;
        return false;
    }
    else if(strategy == latency_strategy::ELIMINATE_DROPS)
        return all_have_data();

    return false;
}

bool fusion_queue::dispatch_next(std::unique_lock<std::mutex> &lock, bool force)
{
    std::lock_guard<std::mutex> data_guard(data_lock);

    if(queue.empty()) return false;

    if(!force && !ok_to_dispatch()) return false;

    sensor_data data = pop_queue();

    lock.unlock();
    uint64_t id = data.id + data.type*MAX_SENSORS;
    data_receiver(std::move(data));
    queue_count[id]--;
    if(dispatch_count.find(id) == dispatch_count.end())
        dispatch_count[id] = 1;
    else
        dispatch_count[id]++;
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

