#include "gtest/gtest.h"
#include "sensor_fusion_queue.h"
#include "util.h"

TEST(SensorFusionQueue, Reorder)
{
    sensor_clock::time_point last_time;
    auto camf = [&last_time](image_gray8 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto depthf = [&last_time](image_depth16 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto accf = [&last_time](accelerometer_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto gyrf = [&last_time](gyro_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    
    fusion_queue q(camf, depthf, accf, gyrf, fusion_queue::latency_strategy::BALANCED, std::chrono::microseconds(5000));
    
    q.start_sync(true);

    gyro_data g;
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(0));
    q.receive_gyro(std::move(g));

    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(10000));
    q.receive_gyro(std::move(g));

    accelerometer_data a;
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(8000));
    q.receive_accelerometer(std::move(a));

    image_depth16 d;
    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q.receive_depth(std::move(d));

    image_gray8 c;
    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q.receive_camera(std::move(c));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(18000));
    q.receive_accelerometer(std::move(a));

    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(20000));
    q.receive_gyro(std::move(g));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(30000));
    q.receive_gyro(std::move(g));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(28000));
    q.receive_accelerometer(std::move(a));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q.receive_accelerometer(std::move(a));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(40000));
    q.receive_gyro(std::move(g));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(48000));
    q.receive_accelerometer(std::move(a));
    
    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q.receive_camera(std::move(c));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(50000));
    q.receive_gyro(std::move(g));
    
    q.stop_sync();
}

TEST(SensorFusionQueue, Threading)
{
    sensor_clock::time_point last_cam_time;
    sensor_clock::time_point last_acc_time;
    sensor_clock::time_point last_gyr_time;
    
    //Times in us
    //Thread time isn't really how long we'll spend since we just sleep for interval us
    auto thread_time = std::chrono::microseconds(100000);
    const sensor_clock::duration camera_interval = std::chrono::microseconds(66);
    const sensor_clock::duration inertial_interval = std::chrono::microseconds(20);
    const sensor_clock::duration jitter = std::chrono::microseconds(10);
    const sensor_clock::duration cam_latency = std::chrono::microseconds(10);
    const sensor_clock::duration in_latency = std::chrono::microseconds(2);
    
    int camsent = 0;
    int gyrsent = 0;
    int accsent = 0;
    
    int camrcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    
    auto camf = [&last_cam_time, &camrcv](image_gray8 &&x) { EXPECT_GE(x.timestamp, last_cam_time); last_cam_time = x.timestamp; ++camrcv; };
    auto depthf = [&last_cam_time, &camrcv](image_depth16 &&x) { EXPECT_GE(x.timestamp, last_cam_time); last_cam_time = x.timestamp; ++camrcv; };
    auto accf = [&last_acc_time, &accrcv](accelerometer_data &&x) { EXPECT_GE(x.timestamp, last_acc_time); last_acc_time = x.timestamp; ++accrcv; };
    auto gyrf = [&last_gyr_time, &gyrrcv](gyro_data &&x) { EXPECT_GE(x.timestamp, last_gyr_time); last_gyr_time = x.timestamp; ++gyrrcv; };

    fusion_queue q(camf, depthf, accf, gyrf, fusion_queue::latency_strategy::BALANCED, jitter);

    auto start = sensor_clock::now();
    
    q.start_sync(true);
    
    std::thread camthread([&q, start, camera_interval, cam_latency, &camsent, thread_time]{
        image_gray8 x;
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            x.timestamp = sensor_clock::time_point(duration);
            std::this_thread::sleep_for(cam_latency);
            q.receive_camera(std::move(x));
            ++camsent;
            std::this_thread::sleep_until(now + camera_interval);
        }
    });
    std::thread gyrothread([&q, start, inertial_interval, in_latency, &gyrsent, thread_time]{
        gyro_data x;
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            x.timestamp = sensor_clock::time_point(duration);
            std::this_thread::sleep_for(in_latency);
            q.receive_gyro(std::move(x));
            ++gyrsent;
            std::this_thread::sleep_until(now + inertial_interval);
        }
    });
    std::thread accelthread([&q, start, inertial_interval, in_latency, &accsent, thread_time]{
        accelerometer_data x;
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            x.timestamp = sensor_clock::time_point(duration);
            std::this_thread::sleep_for(in_latency);
            q.receive_accelerometer(std::move(x));
            ++accsent;
            std::this_thread::sleep_until(now + inertial_interval);
        }
    });
    
    camthread.join();
    gyrothread.join();
    accelthread.join();
    q.stop_sync();
}

TEST(ThreadedDispatch, DropLate)
{
    sensor_clock::time_point last_time;
    auto camf = [&last_time](image_gray8 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto depthf = [&last_time](image_depth16 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto accf = [&last_time](accelerometer_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; EXPECT_NE(x.timestamp, sensor_clock::time_point(std::chrono::microseconds(17000))); };
    auto gyrf = [&last_time](gyro_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; EXPECT_NE(x.timestamp, sensor_clock::time_point(std::chrono::microseconds(30000))); };
    
    fusion_queue q(camf, depthf, accf, gyrf, fusion_queue::latency_strategy::BALANCED, std::chrono::microseconds(5000));
    
    q.start_sync(true);
    
    gyro_data g;
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(0));
    q.receive_gyro(std::move(g));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(10000));
    q.receive_gyro(std::move(g));
    
    accelerometer_data a;
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(8000));
    q.receive_accelerometer(std::move(a));
    
    image_gray8 c;
    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q.receive_camera(std::move(c));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(18000));
    q.receive_accelerometer(std::move(a));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(20000));
    q.receive_gyro(std::move(g));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(19000));
    q.receive_accelerometer(std::move(a));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(28000));
    q.receive_accelerometer(std::move(a));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q.receive_accelerometer(std::move(a));
    
    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q.receive_camera(std::move(c));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(48000));
    q.receive_accelerometer(std::move(a));
    
    //NOTE: This makes this test a little non-deterministic - if it fails due to the 30000 timestamp showing up this could be why
    //If we remove the sleep, everything gets into the queue before the dispatch thread even starts, so everything shows up on the other side in order
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(30000));
    q.receive_gyro(std::move(g));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(40000));
    q.receive_gyro(std::move(g));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(50000));
    q.receive_gyro(std::move(g));
    
    q.stop_sync();
}

