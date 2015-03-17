#include "gtest/gtest.h"
#include "threaded_dispatch.h"
#include "util.h"

TEST(ThreadedDispatch, Reorder)
{
    uint64_t last_time = 0;
    auto camf = [&last_time](const camera_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto accf = [&last_time](const accelerometer_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto gyrf = [&last_time](const gyro_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    
    fusion_queue q(camf, accf, gyrf);
    
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

TEST(ThreadedDispatch, Threading)
{
    uint64_t last_cam_time = 0;
    uint64_t last_acc_time = 0;
    uint64_t last_gyr_time = 0;
    
    //Times in us
    //Thread time isn't really how long we'll spend since we just sleep for interval us
    const uint64_t thread_time = 100000;
    const uint64_t camera_interval = 33;
    const uint64_t inertial_interval = 10;
    
    int camcount = thread_time / camera_interval;
    int gyrcount = thread_time / inertial_interval;
    int acccount = thread_time / inertial_interval;
    
    auto camf = [&last_cam_time, &camcount](const camera_data &x) { EXPECT_GE(x.timestamp, last_cam_time); last_cam_time = x.timestamp; --camcount; };
    auto accf = [&last_acc_time, &acccount](const accelerometer_data &x) { EXPECT_GE(x.timestamp, last_acc_time); last_acc_time = x.timestamp; --acccount; };
    auto gyrf = [&last_gyr_time, &gyrcount](const gyro_data &x) { EXPECT_GE(x.timestamp, last_gyr_time); last_gyr_time = x.timestamp; --gyrcount; };

    fusion_queue q(camf, accf, gyrf);

    auto start = std::chrono::steady_clock::now();
    
    q.start(true);
    
    std::thread camthread([&q, start, camera_interval, camcount]{
        camera_data x;
        for(int i = 0; i < camcount; ++i)
        {
            auto now = std::chrono::steady_clock::now();
            auto duration = now - start;
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            x.timestamp = micros;
            q.receive_camera(std::move(x));
            std::this_thread::sleep_for(std::chrono::microseconds(camera_interval));
        }
    });
    std::thread gyrothread([&q, start, inertial_interval, gyrcount]{
        gyro_data x;
        for(int i = 0; i < gyrcount; ++i)
        {
            auto now = std::chrono::steady_clock::now();
            auto duration = now - start;
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            x.timestamp = micros;
            q.receive_gyro(std::move(x));
            std::this_thread::sleep_for(std::chrono::microseconds(inertial_interval));
        }
    });
    std::thread accelthread([&q, start, inertial_interval, gyrcount]{
        accelerometer_data x;
        for(int i = 0; i < gyrcount; ++i)
        {
            auto now = std::chrono::steady_clock::now();
            auto duration = now - start;
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            x.timestamp = micros;
            q.receive_accelerometer(std::move(x));
            std::this_thread::sleep_for(std::chrono::microseconds(inertial_interval));
        }
    });
    
    camthread.join();
    gyrothread.join();
    accelthread.join();
    q.stop();
    q.wait_until_finished();
    fprintf(stderr, "%d items dropped\n", camcount + gyrcount + acccount);
}

