//
//  threaded_dispatch.h
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#ifndef __threaded_dispatch__
#define __threaded_dispatch__

#include <array>
#include <mutex>
#include <thread>
#include <atomic>

template<typename T, int size>
class sensor_queue
{
public:
    sensor_queue(std::mutex &mx, std::condition_variable &cnd, uint64_t expected_period, uint64_t max_jitter);
    bool push(const T& x); //Doesn't block. Returns false if the queue is full or data arrived out of order
    T pop(); // assumes the lock is already held
    bool empty() const { return count; }
    bool ok_to_dispatch(const uint64_t time) const;
    uint64_t get_next_time() const { return count ? storage[readpos].timestamp : UINT64_MAX; }
    
private:
    std::array<T, size> storage;

    std::mutex &mutex;
    std::condition_variable &cond;

    static_assert(ATOMIC_INT_LOCK_FREE, "This assumes that basic operations on ints are atomic!\n");
    uint64_t last_time;
    uint64_t period;
    uint64_t jitter;
    int readpos;
    int writepos;
    int count;
};

struct accelerometer_data
{
    uint64_t timestamp;
    float accel_m__s2[3];
};

struct gyro_data
{
    uint64_t timestamp;
    float angvel_rad__s[3];
};

struct camera_data
{
    uint64_t timestamp;
    const char *image;
    void *image_handle;
    int width, height, stride;
};

class fusion_queue
{
public:
    fusion_queue(const std::function<void(const camera_data &)> &camera_func,
                 const std::function<void(const accelerometer_data &)> &accelerometer_func,
                 const std::function<void(const gyro_data &)> &gyro_func,
                 uint64_t camera_period,
                 uint64_t inertial_period,
                 uint64_t max_jitter);
    void dispatch();
    void dispatch_next();
    bool can_dispatch();
    void send_control();
    
private:
    std::mutex mutex;
    std::condition_variable cond;
    
    std::function<void(camera_data)> camera_receiver;
    std::function<void(accelerometer_data)> accel_receiver;
    std::function<void(gyro_data)> gyro_receiver;
    
    sensor_queue<accelerometer_data, 10> accel_queue;
    sensor_queue<gyro_data, 10> gyro_queue;
    sensor_queue<camera_data, 3> camera_queue;
    std::function<void()> control_func;
    bool active;
};

#endif /* defined(__threaded_dispatch__) */
