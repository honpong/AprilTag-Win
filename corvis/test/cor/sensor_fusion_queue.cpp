#include "gtest/gtest.h"
#include "sensor_fusion_queue.h"
#include "util.h"

TEST(SensorFusionQueue, Reorder)
{
    uint64_t last_time = 0;
    auto camf = [&last_time](const camera_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto accf = [&last_time](const accelerometer_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto gyrf = [&last_time](const gyro_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    
    fusion_queue q(camf, accf, gyrf, 33000, 10000, 5000);
    
    q.start(true);

    gyro_data g;
    g.timestamp = 0;
    q.receive_gyro(std::move(g));

    g.timestamp = 10000;
    q.receive_gyro(std::move(g));

    accelerometer_data a;
    a.timestamp = 8000;
    q.receive_accelerometer(std::move(a));

    camera_data c;
    c.timestamp = 5000;
    q.receive_camera(std::move(c));

    a.timestamp = 18000;
    q.receive_accelerometer(std::move(a));

    g.timestamp = 20000;
    q.receive_gyro(std::move(g));
    
    g.timestamp = 30000;
    q.receive_gyro(std::move(g));

    a.timestamp = 28000;
    q.receive_accelerometer(std::move(a));

    a.timestamp = 38000;
    q.receive_accelerometer(std::move(a));
    
    g.timestamp = 40000;
    q.receive_gyro(std::move(g));

    a.timestamp = 48000;
    q.receive_accelerometer(std::move(a));
    
    c.timestamp = 38000;
    q.receive_camera(std::move(c));
    
    g.timestamp = 50000;
    q.receive_gyro(std::move(g));
    
    q.stop();
    q.wait_until_finished();
}

TEST(SensorFusionQueue, Threading)
{
    uint64_t last_cam_time = 0;
    uint64_t last_acc_time = 0;
    uint64_t last_gyr_time = 0;
    
    //Times in us
    //Thread time isn't really how long we'll spend since we just sleep for interval us
    auto thread_time = std::chrono::microseconds(100000);
    const uint64_t camera_interval = 66;
    const uint64_t inertial_interval = 20;
    const uint64_t jitter = 10;
    const uint64_t cam_latency = 10;
    const uint64_t in_latency = 2;
    
    int camsent = 0;
    int gyrsent = 0;
    int accsent = 0;
    
    int camrcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    
    auto camf = [&last_cam_time, &camrcv](const camera_data &x) { EXPECT_GE(x.timestamp, last_cam_time); last_cam_time = x.timestamp; ++camrcv; };
    auto accf = [&last_acc_time, &accrcv](const accelerometer_data &x) { EXPECT_GE(x.timestamp, last_acc_time); last_acc_time = x.timestamp; ++accrcv; };
    auto gyrf = [&last_gyr_time, &gyrrcv](const gyro_data &x) { EXPECT_GE(x.timestamp, last_gyr_time); last_gyr_time = x.timestamp; ++gyrrcv; };

    fusion_queue q(camf, accf, gyrf, camera_interval, inertial_interval, jitter);

    auto start = std::chrono::steady_clock::now();
    
    q.start(true);
    
    std::thread camthread([&q, start, camera_interval, cam_latency, &camsent, thread_time]{
        camera_data x;
        auto now = std::chrono::steady_clock::now();
        auto start_time = now;
        while((now = std::chrono::steady_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            x.timestamp = micros;
            if(cam_latency) std::this_thread::sleep_for(std::chrono::microseconds(cam_latency));
            q.receive_camera(std::move(x));
            ++camsent;
            std::this_thread::sleep_until(now + std::chrono::microseconds(camera_interval));
        }
    });
    std::thread gyrothread([&q, start, inertial_interval, in_latency, &gyrsent, thread_time]{
        gyro_data x;
        auto now = std::chrono::steady_clock::now();
        auto start_time = now;
        while((now = std::chrono::steady_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            x.timestamp = micros;
            if(in_latency) std::this_thread::sleep_for(std::chrono::microseconds(in_latency));
            q.receive_gyro(std::move(x));
            ++gyrsent;
            std::this_thread::sleep_until(now + std::chrono::microseconds(inertial_interval));
        }
    });
    std::thread accelthread([&q, start, inertial_interval, in_latency, &accsent, thread_time]{
        accelerometer_data x;
        auto now = std::chrono::steady_clock::now();
        auto start_time = now;
        while((now = std::chrono::steady_clock::now()) < start_time + thread_time)
        {
            auto now = std::chrono::steady_clock::now();
            auto duration = now - start;
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            x.timestamp = micros;
            if(in_latency) std::this_thread::sleep_for(std::chrono::microseconds(in_latency));
            q.receive_accelerometer(std::move(x));
            ++accsent;
            std::this_thread::sleep_until(now + std::chrono::microseconds(inertial_interval));
        }
    });
    
    camthread.join();
    gyrothread.join();
    accelthread.join();
    q.stop();
    q.wait_until_finished();
}

TEST(ThreadedDispatch, DropLate)
{
    uint64_t last_time = 0;
    auto camf = [&last_time](const camera_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto accf = [&last_time](const accelerometer_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; EXPECT_NE(x.timestamp, 17000); };
    auto gyrf = [&last_time](const gyro_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; EXPECT_NE(x.timestamp, 30000); };
    
    fusion_queue q(camf, accf, gyrf, 33000, 10000, 5000);
    
    q.start(true);
    
    gyro_data g;
    g.timestamp = 0;
    q.receive_gyro(std::move(g));
    
    g.timestamp = 10000;
    q.receive_gyro(std::move(g));
    
    accelerometer_data a;
    a.timestamp = 8000;
    q.receive_accelerometer(std::move(a));
    
    camera_data c;
    c.timestamp = 5000;
    q.receive_camera(std::move(c));
    
    a.timestamp = 18000;
    q.receive_accelerometer(std::move(a));
    
    g.timestamp = 20000;
    q.receive_gyro(std::move(g));
    
    a.timestamp = 19000;
    q.receive_accelerometer(std::move(a));
    
    a.timestamp = 28000;
    q.receive_accelerometer(std::move(a));
    
    a.timestamp = 38000;
    q.receive_accelerometer(std::move(a));
    
    c.timestamp = 38000;
    q.receive_camera(std::move(c));
    
    a.timestamp = 48000;
    q.receive_accelerometer(std::move(a));
    
    //NOTE: This makes this test a little non-deterministic - if it fails due to the 30000 timestamp showing up this could be why
    //If we remove the sleep, everything gets into the queue before the dispatch thread even starts, so everything shows up on the other side in order
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    g.timestamp = 30000;
    q.receive_gyro(std::move(g));
    
    g.timestamp = 40000;
    q.receive_gyro(std::move(g));
    
    g.timestamp = 50000;
    q.receive_gyro(std::move(g));
    
    q.stop();
    q.wait_until_finished();
}

