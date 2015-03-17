//
//  threaded_dispatch.cpp
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

//#include "intel_interface.h"
#include "threaded_dispatch.h"
#include <cassert>

template<typename T, int size>
sensor_queue<T, size>::sensor_queue(std::mutex &mx, std::condition_variable &cnd, const bool &actv): mutex(mx), cond(cnd), active(actv), readpos(0), writepos(0), count(0)
{
}

template<typename T, int size>
bool sensor_queue<T, size>::push(T&& x)
{
    std::unique_lock<std::mutex> lock(mutex);
    if(!active)
    {
        lock.unlock();
        return false;
    }
    
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

fusion_queue::fusion_queue(const std::function<void(const camera_data &)> &camera_func,
                           const std::function<void(const accelerometer_data &)> &accelerometer_func,
                           const std::function<void(const gyro_data &)> &gyro_func):
                camera_receiver(camera_func),
                accel_receiver(accelerometer_func),
                gyro_receiver(gyro_func),
                accel_queue(mutex, cond, active),
                gyro_queue(mutex, cond, active),
                camera_queue(mutex, cond, active),
                control_func(nullptr),
                active(false)
{
}

bool fusion_queue::can_dispatch(std::unique_lock<std::mutex> &lock)
{
    return
        camera_queue.get_next_time(lock) != UINT64_MAX ||
        accel_queue.get_next_time(lock) != UINT64_MAX ||
        gyro_queue.get_next_time(lock) != UINT64_MAX;
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
        lock.unlock();
        accel_receiver(std::move(data));
        lock.lock();
    }
    else
    {
        gyro_data data = gyro_queue.pop(lock);
        lock.unlock();
        gyro_receiver(std::move(data));
        lock.lock();
    }
}
