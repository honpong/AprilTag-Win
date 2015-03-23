//
//  sensor_fusion_queue.cpp
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "sensor_fusion_queue.h"
#include <cassert>

template<typename T, int size>
sensor_queue<T, size>::sensor_queue(std::mutex &mx, std::condition_variable &cnd, const bool &actv, const uint64_t expected_period): period(expected_period), last_in(0), last_out(0), mutex(mx), cond(cnd), active(actv), readpos(0), writepos(0), count(0)
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
    
    uint64_t time = x.timestamp;
#ifdef DEBUG
    assert(time >= last_in);
#endif
    uint64_t delta = time - last_in;
    const float alpha = .1;
    period = alpha * delta + (1. - alpha) * period;
    last_in = time;

    ++total_in;

    if(count == size) {
        pop(lock);
        ++drop_full;
    }
    
    storage[writepos] = std::move(x);
    writepos = (writepos + 1) % size;
    ++count;
    
    lock.unlock();
    cond.notify_one();
    return true;
}

template<typename T, int size>
uint64_t sensor_queue<T, size>::get_next_time(const std::unique_lock<std::mutex> &lock, uint64_t last_global_time)
{
    while(count && storage[readpos].timestamp < last_global_time) {
        pop(lock);
        ++drop_late;
    }
    return count ? storage[readpos].timestamp : UINT64_MAX;
}

template<> uint64_t sensor_queue<camera_data, 8>::get_next_time(const std::unique_lock<std::mutex> &lock, uint64_t last_global_time)
{
    while(count && storage[readpos].timestamp < last_global_time) {
        pop(lock);
        ++drop_late;
    }
    return count ? storage[readpos].timestamp : UINT64_MAX;
}


//NOTE: this doesn't actually do anything with the lock - we just ask for it to make sure the caller owns it
template<typename T, int size>
T sensor_queue<T, size>::pop(const std::unique_lock<std::mutex> &lock)
{
#ifdef DEBUG
    assert(count);
#endif

    last_out = storage[readpos].timestamp;
    --count;
    ++total_out;

    int oldpos = readpos;
    readpos = (readpos + 1) % size;
    return std::move(storage[oldpos]);
}

fusion_queue::fusion_queue(const std::function<void(const camera_data &)> &camera_func,
                           const std::function<void(const accelerometer_data &)> &accelerometer_func,
                           const std::function<void(const gyro_data &)> &gyro_func,
                           uint64_t cam_period,
                           uint64_t inertial_period,
                           uint64_t max_jitter):
                camera_receiver(camera_func),
                accel_receiver(accelerometer_func),
                gyro_receiver(gyro_func),
                accel_queue(mutex, cond, active, inertial_period),
                gyro_queue(mutex, cond, active, inertial_period),
                camera_queue(mutex, cond, active, cam_period),
                control_func(nullptr),
                active(false),
                camera_period_expected(cam_period),
                inertial_period_expected(inertial_period),
                last_dispatched(0),
                jitter(max_jitter)
{
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

bool fusion_queue::run_control(const std::unique_lock<std::mutex>& lock)
{
    if(!control_func) return false;
    control_func();
    control_func = nullptr;
    return true;
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
        while(active && !run_control(lock) && !dispatch_next(lock, false))
        {
            cond.wait(lock);
        }
        while(run_control(lock) || dispatch_next(lock, false)); //we need to be greedy, since we only get woken on new data arriving
    }
    //flush any remaining data
    while (dispatch_next(lock, true));
    lock.unlock();
    fprintf(stderr, "Camera: "); camera_queue.print_stats();
    fprintf(stderr, "Accel: "); accel_queue.print_stats();
    fprintf(stderr, "Gyro: "); gyro_queue.print_stats();
}

bool fusion_queue::dispatch_next(std::unique_lock<std::mutex> &lock, bool force)
{
    uint64_t camera_time = camera_queue.get_next_time(lock, last_dispatched);
    uint64_t accel_time = accel_queue.get_next_time(lock, last_dispatched);
    uint64_t gyro_time = gyro_queue.get_next_time(lock, last_dispatched);
    
    if(camera_time <= accel_time && camera_time <= gyro_time)
    {
        if(camera_time == UINT64_MAX) return false; // Only need this in one case because comparison is <=, so == case results in camera data every time
        /*
         We would process an image as soon as we get it because:
         -We can assume that camera latency is longer than gyro/accel latency
         -A dropped inertial sample is less critical than a dropped camera frame
         -Camera processing is most expensive, so we should always start it as soon as we can
         However, the latency assumption does not appear to be true on iOS
         */
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
        //we always dispatch if we are forced, or if we are blocking a camera frame
        if(!force && camera_time == UINT64_MAX)
        {
            //Don't dispatch if we haven't got the next piece of data from the other queues and we are close to or later than their expected arrival, always wait for the first camera frame
            if(camera_queue.last_in == 0) return false;
            if(accel_time > camera_queue.last_out + camera_period_expected - jitter) return false;
            if(gyro_time == UINT64_MAX && accel_time > gyro_queue.last_out + gyro_queue.period - jitter) return false;
        }
        
        accelerometer_data data = accel_queue.pop(lock);
        last_dispatched = data.timestamp;
        lock.unlock();
        accel_receiver(std::move(data));
        lock.lock();
    }
    else
    {
        //we always dispatch if we are forced, or if we are blocking a camera frame
        if(!force && camera_time == UINT64_MAX)
        {
            //Don't dispatch if we haven't got the next piece of data from the other queues and we are close to or later than their expected arrival, always wait for the first camera frame
            if(camera_queue.last_in == 0) return false;
            if(gyro_time > camera_queue.last_out + camera_period_expected - jitter) return false;
            if(accel_time == UINT64_MAX && gyro_time > accel_queue.last_out + accel_queue.period - jitter) return false;
        }

        gyro_data data = gyro_queue.pop(lock);
        last_dispatched = data.timestamp;
        lock.unlock();
        gyro_receiver(std::move(data));
        lock.lock();
    }
    return true;
}
