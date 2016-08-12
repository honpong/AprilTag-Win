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
#include "sensor_data.h"
#include "platform/sensor_clock.h"
#include "../numerics/vec4.h"
#include <iostream>
#include <string>

template<typename T, int size>
class sensor_queue
{
public:
    sensor_queue(std::mutex &mx, std::condition_variable &cnd, const bool &actv);
    void reset();
    bool empty() const { return count == 0; }
    bool full() const { return count == size; }
    bool push(T&& x); //Doesn't block. Returns false if the queue is full or data arrived out of order
    T pop(const std::unique_lock<std::mutex> &lock); // assumes the lock is already held
    sensor_clock::time_point get_next_time(const std::unique_lock<std::mutex> &lock, sensor_clock::time_point last_global_dispatched);

    std::chrono::duration<double, std::micro> period;
    sensor_clock::time_point last_in;
    sensor_clock::time_point last_out;
    
    uint64_t drop_full = 0;
    uint64_t drop_late = 0;
    uint64_t total_in = 0;
    uint64_t total_out = 0;
    stdev<1> stats;
#ifdef DEBUG
    histogram hist{200};
#endif

    std::string get_stats()
    {
        std::ostringstream os;
        os << "period (moving average) " << period.count() << " total in " << total_in << " total out " << total_out << " drop full " << drop_full << " drop late " << drop_late << " timing " << stats;
        return os.str();
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
        IMAGE_TRIGGER, //buffer data until we get an image, then process everything befoer that image. if we don't expect images, then behave like minimize drops
        BALANCED, //Blends strategy of minimize drops when we aren't blocking vision processing and minimize latency when we are blocking vision. Generally low rate, <5% of dropped data.
        MINIMIZE_DROPS, //we'll only drop if something arrives earlier than expected. Almost never drops
        ELIMINATE_DROPS //we always wait until the data in the other queues is ready
        //Both minimize_drops and eliminate_drops may exhibit spurious drops due to different latencies in startup time of sensors. Since they wait for future data to show up, if that other data stream hasn't started yet, then the other buffers can fill up and drop due to being full.
    };

    fusion_queue(const std::function<void(image_gray8 &&)> &camera_func,
                 const std::function<void(image_depth16 &&)> &depth_func,
                 const std::function<void(accelerometer_data &&)> &accelerometer_func,
                 const std::function<void(gyro_data &&)> &gyro_func,
                 latency_strategy s,
                 sensor_clock::duration max_jitter);
    ~fusion_queue();
    
    void reset();
    void start_async(bool expect_camera);
    void start_sync(bool expect_camera);
    void start_singlethreaded(bool expect_camera);
    void start_buffering();
    void stop_immediately();
    void stop_async();
    void stop_sync();
    void wait_until_finished();
    std::string get_stats();

    void receive_camera(image_gray8&& x);
    void receive_depth(image_depth16&& x);
    void receive_accelerometer(accelerometer_data&& x);
    void receive_gyro(gyro_data&& x);
    void dispatch_sync(std::function<void()> fn);
    void dispatch_async(std::function<void()> fn);

    latency_strategy strategy;

private:
    void runloop();
    bool run_control();
    bool ok_to_dispatch(sensor_clock::time_point time);
    bool dispatch_next(std::unique_lock<std::mutex> &lock, bool force);
    void dispatch_singlethread(bool force);
    void dispatch_buffer();
    sensor_clock::time_point global_latest_received() const;

    std::mutex mutex;
    std::condition_variable cond;
    std::thread thread;
    
    std::function<void(image_gray8 &&)> camera_receiver;
    std::function<void(image_depth16 &&)> depth_receiver;
    std::function<void(accelerometer_data &&)> accel_receiver;
    std::function<void(gyro_data &&)> gyro_receiver;
    
    sensor_queue<image_gray8, 6> camera_queue;
    sensor_queue<image_depth16, 6> depth_queue;
    sensor_queue<accelerometer_data, 64> accel_queue;
    sensor_queue<gyro_data, 64> gyro_queue;
    std::function<void()> control_func;
    bool active;
    bool wait_for_camera;
    bool singlethreaded;
    
    sensor_clock::time_point last_dispatched;
    
    sensor_clock::duration jitter;
};

#endif /* defined(__sensor_fusion_queue__) */
