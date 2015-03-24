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
#include "platform/sensor_data.h"

template<typename T, int size>
class sensor_queue
{
public:
    sensor_queue(std::mutex &mx, std::condition_variable &cnd, const bool &actv, const uint64_t expected_period);
    bool empty() const { return count == 0; }
    bool push(T&& x); //Doesn't block. Returns false if the queue is full or data arrived out of order
    T pop(const std::unique_lock<std::mutex> &lock); // assumes the lock is already held
    uint64_t get_next_time(const std::unique_lock<std::mutex> &lock, uint64_t last_global_dispatched);

    uint64_t period;
    uint64_t last_in;
    uint64_t last_out;
    
    uint64_t drop_full = 0;
    uint64_t drop_late = 0;
    uint64_t total_in = 0;
    uint64_t total_out = 0;

    void print_stats()
    {
        fprintf(stderr, "period %lld, total in %lld, total out %lld, drop full %lld, drop late %lld\n", period, total_in, total_out, drop_full, drop_late);
    }
    
private:
    std::array<T, size> storage;

    std::mutex &mutex;
    std::condition_variable &cond;
    const bool &active;
    
    int readpos;
    int writepos;
    std::atomic<int> count;
};

/*
 Intention is that this fusion queue outputs a causal stream of data:
 A measurement with timestamp t is *complete* at that time, and all measurements are delivered in order relative to these timestamps
 For camera, if global shutter, t should be the middle of the integration period
 If rolling shutter, t should be the end of the integration period.
 */
class fusion_queue
{
public:
    enum class latency_strategy
    {
        ELIMINATE_LATENCY, //Immediate dispatch. Not recommended. May fail entirely depending on relative latencies as all data from one sensor is dropped.
        MINIMIZE_LATENCY, //Only wait if we are less than 1 ms before or less than jitter ms after the expected arrival of future data. Generally results in 10-20% dropped data
        BALANCED, //Blends strategy of minimize drops when we aren't blocking vision processing and minimize latency when we are blocking vision. Generally low rate of dropped data.
        MINIMIZE_DROPS, //we'll only drop if something arrives earlier than expected. Almost never drops, but may drop tdue
        ELIMINATE_DROPS //we always wait until the data in the other queues is ready,
    };

    fusion_queue(const std::function<void(const camera_data &)> &camera_func,
                 const std::function<void(const accelerometer_data &)> &accelerometer_func,
                 const std::function<void(const gyro_data &)> &gyro_func,
                 latency_strategy s,
                 uint64_t camera_period,
                 uint64_t inertial_period,
                 uint64_t max_jitter);
    
    void start_async(bool expect_camera);
    void start_sync(bool expect_camera);
    void stop_async();
    void stop_sync();
    void wait_until_finished();

    void receive_camera(camera_data&& x);
    void receive_accelerometer(accelerometer_data&& x);
    void receive_gyro(gyro_data&& x);
    void dispatch_sync(std::function<void()> fn);
    void dispatch_async(std::function<void()> fn);
    
private:
    void runloop();
    bool run_control(const std::unique_lock<std::mutex> &lock);
    bool ok_to_dispatch(uint64_t time);
    bool dispatch_next(std::unique_lock<std::mutex> &lock, bool force);
    uint64_t global_latest_received() const;

    std::mutex mutex;
    std::condition_variable cond;
    std::thread thread;
    
    std::function<void(camera_data)> camera_receiver;
    std::function<void(accelerometer_data)> accel_receiver;
    std::function<void(gyro_data)> gyro_receiver;
    
    sensor_queue<accelerometer_data, 32> accel_queue;
    sensor_queue<gyro_data, 32> gyro_queue;
    sensor_queue<camera_data, 8> camera_queue;
    std::function<void()> control_func;
    bool active;
    bool wait_for_camera;
    
    latency_strategy strategy;
    
    uint64_t camera_period_expected;
    uint64_t inertial_period_expected;
    uint64_t last_dispatched;
    
    uint64_t jitter;
};

#endif /* defined(__sensor_fusion_queue__) */
