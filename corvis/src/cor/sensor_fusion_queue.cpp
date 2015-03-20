//
//  sensor_fusion_queue.cpp
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "sensor_fusion_queue.h"
#include <cassert>

template<typename T, int size>
sensor_queue<T, size>::sensor_queue(std::mutex &mx, std::condition_variable &cnd, const bool &actv, uint64_t &latest_received, const uint64_t &last_dispatched, uint64_t expected_period, uint64_t max_jitter): mutex(mx), cond(cnd), active(actv), global_latest_received(latest_received), global_last_dispatched(last_dispatched), last_time(0), period(expected_period), jitter(max_jitter), readpos(0), writepos(0), count(0)
{
}

template<typename T, int size>
bool sensor_queue<T, size>::push(T&& x)
{
    std::unique_lock<std::mutex> lock(mutex);
    if(!active || x.timestamp < last_time || x.timestamp < global_last_dispatched)
    {
        lock.unlock();
        return false;
    }
    
    if(x.timestamp > global_latest_received) global_latest_received = x.timestamp;
    last_time = x.timestamp;
    
    storage[writepos] = std::move(x);
    writepos = (writepos + 1) % size;

    if(count == size)
    {
        readpos = (readpos + 1) % size;
    }
    else
    {
        ++count;
    }
    lock.unlock();
    cond.notify_one();
    return true;
}

//NOTE: this doesn't actually do anything with the lock - we just ask for it to make sure the caller owns it
template<typename T, int size>
T sensor_queue<T, size>::pop(std::unique_lock<std::mutex> &lock)
{
#ifdef DEBUG
    assert(count);
#endif
    --count;
    int oldpos = readpos;
    readpos = (readpos + 1) % size;
    return std::move(storage[oldpos]);
}


//Get this working again, but then move it out to fusion queue and make sure camera latency doesn't invoke global_latest_received thing
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
    if(time <= last_time + period - jitter) return true;
    //we're late and next piece of data will probably be dropped! let this go ahead
    if(global_latest_received > last_time + period + jitter) return true;
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
                accel_queue(mutex, cond, active, latest_received, last_dispatched, inertial_period, max_jitter),
                gyro_queue(mutex, cond, active, latest_received, last_dispatched, inertial_period, max_jitter),
                camera_queue(mutex, cond, active, latest_received, last_dispatched, camera_period, max_jitter),
                control_func(nullptr),
                active(false),
                latest_received(0),
                last_dispatched(0)
{
}

bool fusion_queue::can_dispatch(std::unique_lock<std::mutex> &lock)
{
    uint64_t min_time = camera_queue.get_next_time(lock);
    uint64_t accel_time = accel_queue.get_next_time(lock);
    if(accel_time < min_time) min_time = accel_time;
    uint64_t gyro_time = gyro_queue.get_next_time(lock);
    if(gyro_time < min_time) min_time = gyro_time;
    
    if(min_time == UINT64_MAX) return false; //nothing ready
    
    return(camera_queue.ok_to_dispatch(min_time) &&
           accel_queue.ok_to_dispatch(min_time) &&
           gyro_queue.ok_to_dispatch(min_time));
}

void fusion_queue::receive_camera(camera_data&& x) { camera_queue.push(std::move(x)); }
void fusion_queue::receive_accelerometer(accelerometer_data&& x) { accel_queue.push(std::move(x)); }
void fusion_queue::receive_gyro(gyro_data&& x) { gyro_queue.push(std::move(x)); }

void fusion_queue::dispatch_sync(std::function<void()> fn)
{
    std::unique_lock<std::mutex> lock(mutex);
    fn();
    lock.unlock();
}

void fusion_queue::dispatch_async(std::function<void()> fn)
{
    std::unique_lock<std::mutex> lock(mutex);
    control_func = fn;
    lock.unlock();
}

void fusion_queue::start(bool synchronous)
{
    if(!thread.joinable())
    {
        if(synchronous)
        {
            std::unique_lock<std::mutex> lock(mutex);
            thread = std::thread(&fusion_queue::runloop, this);
            while(!active)
            {
                cond.wait(lock);
            }
        }
        else
        {
            thread = std::thread(&fusion_queue::runloop, this);
        }
    }
}

void fusion_queue::stop(bool synchronous)
{
    std::unique_lock<std::mutex> lock(mutex);
    active = false;
    lock.unlock();
    cond.notify_one();
    if(synchronous) wait_until_finished();
}

void fusion_queue::wait_until_finished()
{
    if(thread.joinable()) thread.join();
}

void fusion_queue::runloop()
{
    std::unique_lock<std::mutex> lock(mutex);
    active = true;
    //If we were launched synchronously, wake up the launcher
    lock.unlock();
    cond.notify_one();
    lock.lock();
    while(active)
    {
        while(active && !control_func && !can_dispatch(lock))
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
            if(can_dispatch(lock))
            {
                dispatch_next(lock);
            }
        } while(control_func || can_dispatch(lock)); //we need to be greedy, since we only get woken on new data arriving
    }
    //flush any remaining data
    while(!camera_queue.empty() || !accel_queue.empty() || !gyro_queue.empty())
    {
        dispatch_next(lock);
    }
    lock.unlock();
}

void fusion_queue::dispatch_next(std::unique_lock<std::mutex> &lock)
{
    uint64_t camera_time = camera_queue.get_next_time(lock);
    uint64_t accel_time = accel_queue.get_next_time(lock);
    uint64_t gyro_time = gyro_queue.get_next_time(lock);
    if(camera_time <= accel_time && camera_time <= gyro_time)
    {
        camera_data data = camera_queue.pop(lock);
        last_dispatched = data.timestamp;
        lock.unlock();
        camera_receiver(std::move(data));
        
        /* In camera_receiver:
         receiver.process_camera(data);
         sensor_fusion_data = receiver.get_sensor_fusion_data();
         async([] {
            caller.sensor_fusion_did_update(data.image_handle, sensor_fusion_data);
            if(!keeping_image) caller.release_image_handle(data.image_handle);
         });
        
         this should be dispatched asynchronously
         implement it delegate style
         The call to release the platform specific image handle should be independent of the callback for two cases:
         1. Processed frame, but didn't update state (dropped)
         2. Need to hang on the the frame in order to do something (for example, run detector)
         */
        lock.lock();
    }
    else if(accel_time <= gyro_time)
    {
        accelerometer_data data = accel_queue.pop(lock);
        last_dispatched = data.timestamp;
        lock.unlock();
        accel_receiver(std::move(data));
        lock.lock();
    }
    else
    {
        gyro_data data = gyro_queue.pop(lock);
        last_dispatched = data.timestamp;
        lock.unlock();
        gyro_receiver(std::move(data));
        lock.lock();
    }
}
