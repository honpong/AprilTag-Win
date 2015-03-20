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
    sensor_queue(std::mutex &mx, std::condition_variable &cnd, const bool &actv);
    bool empty() const { return count == 0; }
    bool push(T&& x); //Doesn't block. Returns false if the queue is full or data arrived out of order
    T pop(std::unique_lock<std::mutex> &lock); // assumes the lock is already held
    uint64_t get_next_time(std::unique_lock<std::mutex> &lock) const { return count ? storage[readpos].timestamp : UINT64_MAX; }
private:
    std::array<T, size> storage;

    std::mutex &mutex;
    std::condition_variable &cond;
    const bool &active;

    int readpos;
    int writepos;
    std::atomic<int> count;
};

class fusion_queue
{
public:
    fusion_queue(const std::function<void(const camera_data &)> &camera_func,
                 const std::function<void(const accelerometer_data &)> &accelerometer_func,
                 const std::function<void(const gyro_data &)> &gyro_func);
    
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
    void dispatch_next(std::unique_lock<std::mutex> &lock);
    bool can_dispatch(std::unique_lock<std::mutex> &lock);

    std::mutex mutex;
    std::condition_variable cond;
    std::thread thread;
    
    std::function<void(camera_data)> camera_receiver;
    std::function<void(accelerometer_data)> accel_receiver;
    std::function<void(gyro_data)> gyro_receiver;
    
    sensor_queue<accelerometer_data, 10> accel_queue;
    sensor_queue<gyro_data, 10> gyro_queue;
    sensor_queue<camera_data, 3> camera_queue;
    std::function<void()> control_func;
    bool active;    
};

#endif /* defined(__sensor_fusion_queue__) */
