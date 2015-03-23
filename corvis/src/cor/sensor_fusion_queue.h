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
    fusion_queue(const std::function<void(const camera_data &)> &camera_func,
                 const std::function<void(const accelerometer_data &)> &accelerometer_func,
                 const std::function<void(const gyro_data &)> &gyro_func,
                 uint64_t camera_period,
                 uint64_t inertial_period,
                 uint64_t max_jitter);
    
    void start(bool synchronous = false);
    void stop(bool synchronous = false);
    void wait_until_finished();

    void receive_camera(camera_data&& x);
    void receive_accelerometer(accelerometer_data&& x);
    void receive_gyro(gyro_data&& x);
    void dispatch_sync(std::function<void()> fn);
    void dispatch_async(std::function<void()> fn);
    
private:
    void runloop();
    bool run_control(const std::unique_lock<std::mutex> &lock);
    bool dispatch_next(std::unique_lock<std::mutex> &lock, bool force);
    uint64_t global_latest_received() const;

    std::mutex mutex;
    std::condition_variable cond;
    std::thread thread;
    
    std::function<void(camera_data)> camera_receiver;
    std::function<void(accelerometer_data)> accel_receiver;
    std::function<void(gyro_data)> gyro_receiver;
    
    sensor_queue<accelerometer_data, 64> accel_queue;
    sensor_queue<gyro_data, 64> gyro_queue;
    sensor_queue<camera_data, 8> camera_queue;
    std::function<void()> control_func;
    bool active;
    
    uint64_t camera_period_expected;
    uint64_t inertial_period_expected;
    uint64_t last_dispatched;
    
    uint64_t jitter;
};

#endif /* defined(__sensor_fusion_queue__) */
