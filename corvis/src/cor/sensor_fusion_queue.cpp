//
//  sensor_fusion_queue.cpp
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "sensor_fusion_queue.h"
#include <cassert>

template<typename T, int size>
sensor_queue<T, size>::sensor_queue(std::mutex &mx, std::condition_variable &cnd, const bool &actv, const sensor_clock::duration expected_period): period(expected_period), mutex(mx), cond(cnd), active(actv), readpos(0), writepos(0), count(0)
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
    
    sensor_clock::time_point time = x.timestamp;
#ifdef DEBUG
    assert(time >= last_in);
#endif
    if(last_in != sensor_clock::time_point())
    {
        sensor_clock::duration delta = time - last_in;
        stats.data(delta.count());
        const float alpha = .1f;
        period = alpha * delta + (1.f - alpha) * period;
    }
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
sensor_clock::time_point sensor_queue<T, size>::get_next_time(const std::unique_lock<std::mutex> &lock, sensor_clock::time_point last_global_dispatched)
{
    while(count && storage[readpos].timestamp < last_global_dispatched) {
        pop(lock);
        ++drop_late;
    }
    return count ? storage[readpos].timestamp : sensor_clock::time_point();
}

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

fusion_queue::fusion_queue(const std::function<void(camera_data &&)> &camera_func,
                           const std::function<void(accelerometer_data &&)> &accelerometer_func,
                           const std::function<void(gyro_data &&)> &gyro_func,
                           latency_strategy s,
                           sensor_clock::duration cam_period,
                           sensor_clock::duration inertial_period,
                           sensor_clock::duration max_jitter):
                camera_receiver(camera_func),
                accel_receiver(accelerometer_func),
                gyro_receiver(gyro_func),
                accel_queue(mutex, cond, active, inertial_period),
                gyro_queue(mutex, cond, active, inertial_period),
                camera_queue(mutex, cond, active, cam_period),
                control_func(nullptr),
                active(false),
                wait_for_camera(true),
                singlethreaded(false),
                strategy(s),
                camera_period_expected(cam_period),
                inertial_period_expected(inertial_period),
                jitter(max_jitter)
{
}

fusion_queue::~fusion_queue()
{
    stop_immediately();
    wait_until_finished();
}


void fusion_queue::receive_camera(camera_data&& x)
{
    camera_queue.push(std::move(x));
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::receive_accelerometer(accelerometer_data&& x)
{
    accel_queue.push(std::move(x));
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::receive_gyro(gyro_data&& x)
{
    gyro_queue.push(std::move(x));
    if(singlethreaded) dispatch_singlethread(false);
}

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
    if(singlethreaded) dispatch_singlethread(false);
}

void fusion_queue::start_async(bool expect_camera)
{
    wait_for_camera = expect_camera;
    if(!thread.joinable())
    {
        thread = std::thread(&fusion_queue::runloop, this);
    }
}

void fusion_queue::start_sync(bool expect_camera)
{
    wait_for_camera = expect_camera;
    if(!thread.joinable())
    {
        std::unique_lock<std::mutex> lock(mutex);
        thread = std::thread(&fusion_queue::runloop, this);
        while(!active)
        {
            cond.wait(lock);
        }
    }
}

void fusion_queue::start_singlethreaded(bool expect_camera)
{
    wait_for_camera = expect_camera;
    singlethreaded = true;
    active = true;
}

void fusion_queue::stop_immediately()
{
    std::unique_lock<std::mutex> lock(mutex);
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
        std::cerr << "Camera: " << camera_queue.get_stats();
        std::cerr << "Accel: " << accel_queue.get_stats();
        std::cerr << "Gyro: " << gyro_queue.get_stats();
#endif
    }
    stop_immediately();
}

void fusion_queue::stop_sync()
{
    stop_async();
    if(!singlethreaded) wait_until_finished();
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
#ifdef DEBUG
    std::cerr << "Camera: " << camera_queue.get_stats();
    std::cerr << "Accel: " << accel_queue.get_stats();
    std::cerr << "Gyro: " << gyro_queue.get_stats();
#endif
    lock.unlock();
}

sensor_clock::time_point  fusion_queue::global_latest_received() const
{
    if(camera_queue.last_in >= gyro_queue.last_in && camera_queue.last_in >= accel_queue.last_in) return camera_queue.last_in;
    else if(accel_queue.last_in >= gyro_queue.last_in) return accel_queue.last_in;
    return gyro_queue.last_in;
}

bool fusion_queue::ok_to_dispatch(sensor_clock::time_point time)
{
    if(strategy == latency_strategy::ELIMINATE_LATENCY) return true; //always dispatch if we are eliminating latency
    
    if(strategy == latency_strategy::IMAGE_TRIGGER && wait_for_camera) //if we aren't waiting for camera, then IMAGE_TRIGGER behaves like MINIMIZE_DROPS
    {
        if(camera_queue.empty()) return false;
        else return true;
    }

    //We test the proposed queue against itself, but immediately green light it because queue won't be empty anyway
    if(camera_queue.empty() && wait_for_camera)
    {
        /*
         Camera gets special treatment because:
         -A dropped inertial sample is less critical than a dropped camera frame
         -Camera processing is most expensive, so we should always start it as soon as we can
         However, we can't go too far, because it turns out that camera latency (including offset) is not significantly longer than gyro/accel latency in iOS
         */
        if(strategy == latency_strategy::ELIMINATE_DROPS) return false;
        if(time > camera_queue.last_out + camera_queue.period - std::chrono::milliseconds(1))
        {
            //If we are in balanced mode, camera gets special treatment to be like minimize_drops
            if(strategy == latency_strategy::BALANCED || strategy == latency_strategy::MINIMIZE_DROPS) return false;
            if(global_latest_received() < camera_queue.last_out + camera_queue.period + jitter) return false;
        }
    }
    
    if(accel_queue.empty())
    {
        if(strategy == latency_strategy::ELIMINATE_DROPS) return false;
        if(time > accel_queue.last_out + accel_queue.period - std::chrono::milliseconds(1))
        {
            if(strategy == latency_strategy::MINIMIZE_DROPS || strategy == latency_strategy::IMAGE_TRIGGER) return false;
            if(strategy == latency_strategy::BALANCED && wait_for_camera && camera_queue.empty()) return false; //In balanced strategy, we wait longer, as long as we aren't blocking a camera frame, otherwise fall through to minimize latency
            if(global_latest_received() < accel_queue.last_out + accel_queue.period + jitter) return false;
        }
    }
    
    if(gyro_queue.empty())
    {
        if(strategy == latency_strategy::ELIMINATE_DROPS) return false;
        if(time > gyro_queue.last_out + gyro_queue.period - std::chrono::milliseconds(1)) //OK to dispatch if it's far enough ahead of when we expect the other
        {
            if(strategy == latency_strategy::MINIMIZE_DROPS || strategy == latency_strategy::IMAGE_TRIGGER) return false;
            if(strategy == latency_strategy::BALANCED && wait_for_camera && camera_queue.empty()) return false; //In balanced strategy, if we aren't holding up a camera frame, wait
            if(global_latest_received() < accel_queue.last_out + accel_queue.period + jitter) return false; //Otherwise (balanced and minimize latency) wait as long as we aren't likely to be late and dropped
        }
    }
    
    return true;
}

bool fusion_queue::dispatch_next(std::unique_lock<std::mutex> &lock, bool force)
{
    sensor_clock::time_point camera_time = camera_queue.get_next_time(lock, last_dispatched);
    sensor_clock::time_point accel_time = accel_queue.get_next_time(lock, last_dispatched);
    sensor_clock::time_point gyro_time = gyro_queue.get_next_time(lock, last_dispatched);
    
    if(!camera_queue.empty() && (accel_queue.empty() || camera_time <= accel_time) && (gyro_queue.empty() || camera_time <= gyro_time))
    {
        if(!force && !ok_to_dispatch(camera_time)) return false;

        camera_data data = camera_queue.pop(lock);
#ifdef DEBUG
        assert(data.timestamp >= last_dispatched);
#endif
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
    else if(!accel_queue.empty() && (gyro_queue.empty() || accel_time <= gyro_time))
    {
        if(!force && !ok_to_dispatch(accel_time)) return false;
        
        accelerometer_data data = accel_queue.pop(lock);
#ifdef DEBUG
        assert(data.timestamp >= last_dispatched);
#endif
        last_dispatched = data.timestamp;
        lock.unlock();
        accel_receiver(std::move(data));
        lock.lock();
    }
    else if(!gyro_queue.empty())
    {
        if(!force && !ok_to_dispatch(gyro_time)) return false;

        gyro_data data = gyro_queue.pop(lock);
#ifdef DEBUG
        assert(data.timestamp >= last_dispatched);
#endif
        last_dispatched = data.timestamp;
        lock.unlock();
        gyro_receiver(std::move(data));
        lock.lock();
    }
    else return false;
    return true;
}

void fusion_queue::dispatch_singlethread(bool force)
{
    std::unique_lock<std::mutex> lock(mutex);
    while(dispatch_next(lock, force)); //always be greedy - could have multiple pieces of data buffered
    lock.unlock();
}

