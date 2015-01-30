//
//  threaded_dispatch.cpp
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "intel_interface.h"
#include "threaded_dispatch.h"
#include <cassert>

template<typename T, int size>
sensor_queue<T, size>::sensor_queue(std::mutex &mx, std::condition_variable &cnd, uint64_t expected_period, uint64_t max_jitter): mutex(mx), cond(cnd), last_time(0), period(expected_period), jitter(max_jitter), readpos(0), writepos(0), count(0)
{
}

template<typename T, int size>
bool sensor_queue<T, size>::push(const T& x)
{
    if(count == size || x.timestamp < last_time) return false;

    std::unique_lock<std::mutex> lock(mutex);
    //these two lines could be outside of the mutex, except that we can flush everything
    storage[writepos] = x;
    writepos = (writepos + 1) % size;

    last_time = x.timestamp;
    ++count;
    lock.unlock();
    cond.notify_one();

    return true;
}

//NOTE: this assumes that the lock is already held - should not be called directly by a client
template<typename T, int size>
T sensor_queue<T, size>::pop()
{
#ifdef DEBUG
    assert(count);
#endif
    --count;
    int oldpos = readpos;
    readpos = (readpos + 1) % size;
    return storage[oldpos];
}

template<typename T, int size>
bool sensor_queue<T, size>::ok_to_dispatch(const uint64_t time) const
{
#ifdef DEBUG
    //This should only be called with the first available item (which could be ours)
    if(count) assert(time <= storage[readpos].timestamp);
#endif
    //if we aren't debugging, let it go even if it's out of order
    if(count) return true;
    //if it's far enough ahead of when we expect our next data, then go ahead
    if(time < last_time + period - jitter) return true;
    //we're late and will be dropped! let this go ahead
    if(time > last_time + period + jitter) return true;
    //otherwise, our next piece of data could be timestamped around the same time, so wait...
    return false;
}

fusion_queue::fusion_queue(const std::function<void(const camera_data &)> &camera_func,
                              const std::function<void(const accelerometer_data &)> &accelerometer_func,
                              const std::function<void(const gyro_data &)> &gyro_func,
                              uint64_t camera_period,
                              uint64_t inertial_period,
                              uint64_t max_jitter):
                camera_receiver(camera_func),
                accel_receiver(accelerometer_func),
                gyro_receiver(gyro_func),
                accel_queue(mutex, cond, inertial_period, max_jitter),
                gyro_queue(mutex, cond, inertial_period, max_jitter),
                camera_queue(mutex, cond, camera_period, max_jitter),
                control_func(nullptr),
                active(false)
{
}

bool fusion_queue::can_dispatch()
{
    if(control_func) return true;

    uint64_t min_time = camera_queue.get_next_time();
    uint64_t accel_time = accel_queue.get_next_time();
    if(accel_time < min_time) min_time = accel_time;
    uint64_t gyro_time = gyro_queue.get_next_time();
    if(gyro_time < min_time) min_time = gyro_time;
    
    if(min_time == UINT64_MAX) return false; //nothing ready
    
    return(camera_queue.ok_to_dispatch(min_time) &&
           accel_queue.ok_to_dispatch(min_time) &&
           gyro_queue.ok_to_dispatch(min_time));
}

void fusion_queue::receive_camera(const camera_data &x) { camera_queue.push(x); }
void fusion_queue::receive_accelerometer(const accelerometer_data &x) { accel_queue.push(x); }
void fusion_queue::receive_gyro(const gyro_data &x) { gyro_queue.push(x); }

void fusion_queue::dispatch()
{
    std::unique_lock<std::mutex> lock(mutex);
    while(!can_dispatch())
    {
        cond.wait(lock);
    }
    do
    {
        if(control_func)
        {
            control_func();
            control_func = nullptr;
        }
        else
        {
            uint64_t camera_time = camera_queue.get_next_time();
            uint64_t accel_time = accel_queue.get_next_time();
            uint64_t gyro_time = gyro_queue.get_next_time();
            if(camera_time <= accel_time && camera_time <= gyro_time)
            {
                camera_data data = camera_queue.pop();
                lock.unlock();
                camera_receiver(data);

                /* In camera_receiver:
                receiver.process_camera(data);
                sensor_fusion_data = receiver.get_sensor_fusion_data();
                //this should be dispatched asynchronously
                //implement it delegate style
                //it should also have an alternate that says finished, but didn't update sensor fusion
                async(caller.sensor_fusion_did_update(data.image_handle, sensor_fusion_data));
                    */
                lock.lock();
            }
            else if(accel_time <= gyro_time)
            {
                accelerometer_data data = accel_queue.pop();
                lock.unlock();
                accel_receiver(data);
                lock.lock();
            }
            else
            {
                gyro_data data = gyro_queue.pop();
                lock.unlock();
                gyro_receiver(data);
                lock.lock();
            }
        }
    }
    while(can_dispatch()); //we need to be greedy, since we only get woken on new data arriving
    lock.unlock();
}
