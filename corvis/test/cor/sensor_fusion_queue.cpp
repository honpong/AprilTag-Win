#include "gtest/gtest.h"
#include "sensor_fusion_queue.h"
#include "util.h"

std::unique_ptr<fusion_queue> setup_queue(std::function<void(sensor_data && x)> dataf, std::function<void(const sensor_data & x, bool catchup)> fast_dataf, fusion_queue::latency_strategy strategy, uint64_t max_latency_us)
{
    std::unique_ptr<fusion_queue> q = std::make_unique<fusion_queue>(dataf, fast_dataf, strategy, std::chrono::microseconds(max_latency_us));
    q->require_sensor(rc_SENSOR_TYPE_IMAGE, 0, std::chrono::microseconds(0));
    q->require_sensor(rc_SENSOR_TYPE_DEPTH, 0, std::chrono::microseconds(0));
    q->require_sensor(rc_SENSOR_TYPE_ACCELEROMETER, 0, std::chrono::microseconds(0));
    q->require_sensor(rc_SENSOR_TYPE_GYROSCOPE, 0, std::chrono::microseconds(0));

    return q;
}

sensor_data depth16_for_time(uint64_t timestamp_us)
{
    sensor_data d(timestamp_us, rc_SENSOR_TYPE_DEPTH, 0 /* id */,
                0 /*rc_Timestamp shutter_time_us */,
                0, 0, 0, /*int width, int height, int stride,*/
                rc_FORMAT_DEPTH16, nullptr, nullptr, nullptr);

    return d;
}

sensor_data gray8_for_time(uint64_t timestamp_us)
{
    sensor_data d(timestamp_us, rc_SENSOR_TYPE_IMAGE, 0 /* id */,
                0 /*rc_Timestamp shutter_time_us */,
                0, 0, 0, /*int width, int height, int stride,*/
                rc_FORMAT_GRAY8, nullptr, nullptr, nullptr);
    return d;
}

sensor_data accel_for_time(uint64_t timestamp_us)
{
    rc_Vector v = {};
    sensor_data d(timestamp_us, rc_SENSOR_TYPE_ACCELEROMETER, 0 /* id */, v);
    return d;
}

sensor_data gyro_for_time(uint64_t timestamp_us)
{
    rc_Vector v = {};
    sensor_data d(timestamp_us, rc_SENSOR_TYPE_GYROSCOPE, 0 /* id */, v);
    return d;
}

TEST(SensorFusionQueue, Reorder)
{
    uint64_t last_time = 0;
    auto dataf = [&last_time](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time);
        last_time = x.time_us;
    };

    auto fast_dataf = [](const sensor_data & x, bool catchup) { };

    auto q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);
    
    q->start(true);

    q->receive_sensor_data(std::move(gyro_for_time(0)));

    q->receive_sensor_data(std::move(gyro_for_time(10000)));

    q->receive_sensor_data(std::move(accel_for_time(8000)));

    q->receive_sensor_data(std::move(depth16_for_time(5000)));

    q->receive_sensor_data(std::move(gray8_for_time(5000)));

    q->receive_sensor_data(std::move(accel_for_time(18000)));

    q->receive_sensor_data(std::move(gyro_for_time(20000)));
    
    q->receive_sensor_data(std::move(gyro_for_time(30000)));

    q->receive_sensor_data(std::move(accel_for_time(28000)));

    q->receive_sensor_data(std::move(accel_for_time(38000)));
    
    q->receive_sensor_data(std::move(gyro_for_time(40000)));

    q->receive_sensor_data(std::move(accel_for_time(48000)));
    
    q->receive_sensor_data(std::move(depth16_for_time(38000)));

    q->receive_sensor_data(std::move(gray8_for_time(38000)));
    
    q->receive_sensor_data(std::move(gyro_for_time(50000)));
    
    q->stop();
    ASSERT_EQ(q->total_in, 15);
    ASSERT_EQ(q->total_out, 15);
}

#include <iostream>

TEST(SensorFusionQueue, FastCatchup)
{
    const uint64_t jitter_us = 10;

    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    int fast_accrcv = 0;
    int fast_gyrrcv = 0;
    int catchup_accrcv = 0;
    int catchup_gyrrcv = 0;
    
    std::unique_ptr<fusion_queue> q;
    
    auto dataf = [&camrcv, &deprcv, &accrcv, &gyrrcv, &q](sensor_data && x) {
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE:
                q->dispatch_buffered_to_fast_path();
                ++camrcv;
                break;
            case rc_SENSOR_TYPE_DEPTH:
                ++deprcv;
                break;
            case rc_SENSOR_TYPE_ACCELEROMETER:
                ++accrcv;
                break;
            case rc_SENSOR_TYPE_GYROSCOPE:
                ++gyrrcv;
                break;
        }
    };
    
    auto fast_dataf = [&fast_accrcv, &fast_gyrrcv, &catchup_accrcv, &catchup_gyrrcv](const sensor_data &x, bool catchup)
    {
        switch(x.type) {
            case rc_SENSOR_TYPE_ACCELEROMETER:
                if(catchup) ++catchup_accrcv;
                else ++fast_accrcv;
                break;
            case rc_SENSOR_TYPE_GYROSCOPE:
                if(catchup) ++catchup_gyrrcv;
                else ++fast_gyrrcv;
                break;
            default:
                break;
        }
    };
    
    q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, jitter_us);


    q->start(false);
    
    q->receive_sensor_data(gyro_for_time(0));
    
    q->receive_sensor_data(gyro_for_time(10000));
    
    q->receive_sensor_data(accel_for_time(8000));
    
    q->receive_sensor_data(depth16_for_time(5000));
    
    EXPECT_EQ(0, deprcv);
    EXPECT_EQ(0, camrcv);
    EXPECT_EQ(0, accrcv);
    EXPECT_EQ(0, gyrrcv);
    EXPECT_EQ(0, catchup_accrcv);
    EXPECT_EQ(0, catchup_gyrrcv);
    EXPECT_EQ(1, fast_accrcv);
    EXPECT_EQ(2, fast_gyrrcv);
    
    q->receive_sensor_data(gray8_for_time(5000));
    
    EXPECT_EQ(1, deprcv);
    EXPECT_EQ(0, camrcv);
    EXPECT_EQ(0, accrcv);
    EXPECT_EQ(1, gyrrcv);
    EXPECT_EQ(0, catchup_accrcv);
    EXPECT_EQ(0, catchup_gyrrcv);
    EXPECT_EQ(1, fast_accrcv);
    EXPECT_EQ(2, fast_gyrrcv);
    
    q->receive_sensor_data(accel_for_time(18000));
    
    q->receive_sensor_data(gyro_for_time(20000));

    q->receive_sensor_data(gyro_for_time(30000));

    q->receive_sensor_data(accel_for_time(28000));
    
    q->receive_sensor_data(accel_for_time(38000));

    q->receive_sensor_data(gyro_for_time(40000));
    
    q->receive_sensor_data(accel_for_time(48000));
    
    q->receive_sensor_data(depth16_for_time(38000));
    
    EXPECT_EQ(1, deprcv);
    EXPECT_EQ(1, camrcv);
    EXPECT_EQ(0, accrcv);
    EXPECT_EQ(1, gyrrcv);
    EXPECT_EQ(5, catchup_accrcv);
    EXPECT_EQ(4, catchup_gyrrcv);
    EXPECT_EQ(5, fast_accrcv);
    EXPECT_EQ(5, fast_gyrrcv);
    
    q->receive_sensor_data(gray8_for_time(38000));
    
    EXPECT_EQ(2, deprcv);
    EXPECT_EQ(1, camrcv);
    EXPECT_EQ(4, accrcv);
    EXPECT_EQ(4, gyrrcv);
    EXPECT_EQ(5, catchup_accrcv);
    EXPECT_EQ(4, catchup_gyrrcv);
    EXPECT_EQ(5, fast_accrcv);
    EXPECT_EQ(5, fast_gyrrcv);
    
    q->receive_sensor_data(gyro_for_time(50000));
    
    q->stop();
}

TEST(SensorFusionQueue, Threading)
{
    uint64_t last_cam_time = 0;
    uint64_t last_dep_time = 0;
    uint64_t last_acc_time = 0;
    uint64_t last_gyr_time = 0;
    
    //Times in us
    //Thread time isn't really how long we'll spend since we just sleep for interval us
    auto thread_time = std::chrono::microseconds(100000);
    const sensor_clock::duration camera_interval = std::chrono::microseconds(66);
    const sensor_clock::duration inertial_interval = std::chrono::microseconds(20);
    const uint64_t jitter_us = 10;
    const sensor_clock::duration cam_latency = std::chrono::microseconds(10);
    const sensor_clock::duration in_latency = std::chrono::microseconds(2);
    
    int camsent = 0;
    int depsent = 0;
    int gyrsent = 0;
    int accsent = 0;
    
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    int fast_accrcv = 0;
    int fast_gyrrcv = 0;

    std::unique_ptr<fusion_queue> q;
    
    auto dataf = [&last_cam_time, &last_dep_time, &last_acc_time, &last_gyr_time, &camrcv, &deprcv, &accrcv, &gyrrcv, &q](sensor_data && x) {
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE:
                EXPECT_GE(x.time_us, last_cam_time);
                last_cam_time = x.time_us;
                q->dispatch_buffered_to_fast_path();
                ++camrcv;
                break;
            case rc_SENSOR_TYPE_DEPTH:
                EXPECT_GE(x.time_us, last_dep_time);
                last_dep_time = x.time_us;
                ++deprcv;
                break;
            case rc_SENSOR_TYPE_ACCELEROMETER:
                EXPECT_GE(x.time_us, last_acc_time);
                last_acc_time = x.time_us;
                ++accrcv;
                break;
            case rc_SENSOR_TYPE_GYROSCOPE:
                EXPECT_GE(x.time_us, last_gyr_time);
                last_gyr_time = x.time_us;
                ++gyrrcv;
                break;
        }
    };
    
    auto fast_dataf = [&fast_accrcv, &fast_gyrrcv](const sensor_data &x, bool catchup)
    {
        if(catchup) return;
        switch(x.type) {
            case rc_SENSOR_TYPE_ACCELEROMETER:
                ++fast_accrcv;
                break;
            case rc_SENSOR_TYPE_GYROSCOPE:
                ++fast_gyrrcv;
                break;
            default:
                break;
        }
    };

    q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, jitter_us);

    auto start = sensor_clock::now();
    
    q->start(true);
    
    std::thread camthread([&q, start, camera_interval, cam_latency, &camsent, thread_time]{
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            std::this_thread::sleep_for(cam_latency);
            q->receive_sensor_data(std::move(gray8_for_time(duration.count())));
            ++camsent;
            std::this_thread::sleep_for(camera_interval-cam_latency);
        }
    });
    std::thread depthread([&q, start, camera_interval, cam_latency, &camsent, thread_time]{
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            std::this_thread::sleep_for(cam_latency);
            q->receive_sensor_data(std::move(depth16_for_time(duration.count())));
            ++camsent;
            std::this_thread::sleep_for(camera_interval-cam_latency);
        }
    });
    std::thread gyrothread([&q, start, inertial_interval, in_latency, &gyrsent, thread_time]{
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            std::this_thread::sleep_for(in_latency);
            q->receive_sensor_data(std::move(gyro_for_time(duration.count())));
            ++gyrsent;
            std::this_thread::sleep_for(inertial_interval-in_latency);
        }
    });
    std::thread accelthread([&q, start, inertial_interval, in_latency, &accsent, thread_time]{
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            std::this_thread::sleep_for(in_latency);
            q->receive_sensor_data(std::move(accel_for_time(duration.count())));
            ++accsent;
            std::this_thread::sleep_for(inertial_interval-in_latency);
        }
    });
    
    camthread.join();
    depthread.join();
    gyrothread.join();
    accelthread.join();
    q->stop();
    EXPECT_EQ(fast_accrcv, accsent);
    EXPECT_EQ(fast_gyrrcv, gyrsent);
}

TEST(SensorFusionQueue, DropOrder)
{
    uint64_t last_time = 0;
    auto dataf = [&last_time](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time);
        last_time = x.time_us;
        if(x.type == rc_SENSOR_TYPE_ACCELEROMETER)
             EXPECT_NE(x.time_us, 4000);
    };

    auto fast_dataf = [](const sensor_data &x, bool catchup) {};

    auto q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);
    
    q->start(true);
    
    q->receive_sensor_data(std::move(gyro_for_time(0)));
    
    q->receive_sensor_data(std::move(accel_for_time(8000)));
    
    q->receive_sensor_data(std::move(gray8_for_time(5000)));

    q->receive_sensor_data(std::move(depth16_for_time(5000)));

    q->receive_sensor_data(std::move(accel_for_time(4000)));
    q->stop();
}

TEST(ThreadedDispatch, DropLate)
{
    uint64_t last_time = 0;
    auto dataf = [&last_time](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time);
        last_time = x.time_us;
        if(x.type == rc_SENSOR_TYPE_GYROSCOPE)
             EXPECT_NE(x.time_us, 30000);
    };

    auto fast_dataf = [](const sensor_data &x, bool catchup) {};

    auto q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::MINIMIZE_LATENCY, 5000);
    
    q->start(true);
    
    q->receive_sensor_data(std::move(gyro_for_time(0)));

    q->receive_sensor_data(std::move(gyro_for_time(10000)));
    
    q->receive_sensor_data(std::move(accel_for_time(8000)));
    
    q->receive_sensor_data(std::move(gray8_for_time(5000)));

    q->receive_sensor_data(std::move(depth16_for_time(5000)));

    q->receive_sensor_data(std::move(accel_for_time(18000)));
    
    q->receive_sensor_data(std::move(gyro_for_time(20000)));
    
    q->receive_sensor_data(std::move(accel_for_time(19000)));
    
    q->receive_sensor_data(std::move(accel_for_time(28000)));
    
    q->receive_sensor_data(std::move(accel_for_time(38000)));
    
    q->receive_sensor_data(std::move(gray8_for_time(38000)));

    q->receive_sensor_data(std::move(depth16_for_time(38000)));
    
    q->receive_sensor_data(std::move(accel_for_time(48000)));
    
    //NOTE: This makes this test a little non-deterministic - if it fails due to the 30000 timestamp showing up this could be why
    //If we remove the sleep, everything gets into the queue before the dispatch thread even starts, so everything shows up on the other side in order
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    q->receive_sensor_data(std::move(gyro_for_time(30000)));
    
    q->receive_sensor_data(std::move(gyro_for_time(40000)));
    
    q->receive_sensor_data(std::move(gyro_for_time(50000)));
    
    q->stop();
}

TEST(SensorFusionQueue, SameTime)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    uint64_t last_time = 0;

    auto dataf = [&last_time, &camrcv, &deprcv, &accrcv, &gyrrcv](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time);
        last_time = x.time_us;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
        }
    };
    
    auto fast_dataf = [](const sensor_data &x, bool catchup) {};

    auto q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);

    q->start(true);

    q->receive_sensor_data(std::move(gyro_for_time(5000)));

    q->receive_sensor_data(std::move(accel_for_time(5000)));

    q->receive_sensor_data(std::move(gray8_for_time(5000)));

    q->receive_sensor_data(std::move(depth16_for_time(5000)));

    q->stop();

    EXPECT_EQ(camrcv, 1);
    EXPECT_EQ(deprcv, 1);
    EXPECT_EQ(gyrrcv, 1);
    EXPECT_EQ(accrcv, 1);
}

TEST(SensorFusionQueue, MaxLatencyDispatch)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    uint64_t last_time = 0;

    auto dataf = [&last_time, &camrcv, &deprcv, &accrcv, &gyrrcv](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time);
        last_time = x.time_us;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
        }
    };
    
    auto fast_dataf = [](const sensor_data &x, bool catchup) {};

    auto q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);

    q->start(true);

    q->receive_sensor_data(std::move(gyro_for_time(5000)));
    q->receive_sensor_data(std::move(gyro_for_time(6000)));
    q->receive_sensor_data(std::move(gyro_for_time(7000)));
    q->receive_sensor_data(std::move(gyro_for_time(8000)));
    q->receive_sensor_data(std::move(gyro_for_time(9000)));

    q->receive_sensor_data(std::move(accel_for_time(5000)));
    q->receive_sensor_data(std::move(accel_for_time(6000)));
    q->receive_sensor_data(std::move(accel_for_time(7000)));
    q->receive_sensor_data(std::move(accel_for_time(8000)));
    q->receive_sensor_data(std::move(accel_for_time(9000)));

    q->receive_sensor_data(std::move(gray8_for_time(5000)));
    // we should dispatch here due to max latency of 5ms
    q->receive_sensor_data(std::move(gray8_for_time(10001)));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // we should drop this, since we will have already dispatched t =
    // 5000
    q->receive_sensor_data(std::move(depth16_for_time(4000)));

    q->stop();

    EXPECT_EQ(camrcv, 2);
    EXPECT_EQ(deprcv, 0);
    EXPECT_EQ(gyrrcv, 5);
    EXPECT_EQ(accrcv, 5);
}

TEST(SensorFusionQueue, BufferNoDispatch)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    uint64_t buffer_time_us = 50e3;
    uint64_t start_time_us = 1e6;
    uint64_t last_time_us = 0;
    uint64_t extra_time_us = 100e3;

    auto dataf = [&last_time_us, &start_time_us, &buffer_time_us, &camrcv, &deprcv, &accrcv, &gyrrcv](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time_us);
        last_time_us = x.time_us;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
        }
    };
    
    auto fast_dataf = [](const sensor_data &x, bool catchup) {};

    auto q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);

    q->start_buffering(std::chrono::microseconds(buffer_time_us));

    uint64_t time_us;
    for(time_us = 0; time_us < start_time_us; time_us += 2000) {
        q->receive_sensor_data(std::move(gyro_for_time(time_us)));
        q->receive_sensor_data(std::move(accel_for_time(time_us)));
        q->receive_sensor_data(std::move(gray8_for_time(time_us)));
        q->receive_sensor_data(std::move(depth16_for_time(time_us)));
    }

    q->stop();

    EXPECT_EQ(camrcv, 0);
    EXPECT_EQ(deprcv, 0);
    EXPECT_EQ(gyrrcv, 0);
    EXPECT_EQ(accrcv, 0);
}

TEST(SensorFusionQueue, Buffering)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    uint64_t buffer_time_us = 50e3;
    uint64_t start_time_us = 1e6;
    uint64_t last_time_us = 0;
    uint64_t extra_time_us = 100e3;

    auto dataf = [&last_time_us, &start_time_us, &buffer_time_us, &camrcv, &deprcv, &accrcv, &gyrrcv](sensor_data && x) {
        EXPECT_GE(x.time_us, start_time_us - buffer_time_us);
        EXPECT_GE(x.time_us, last_time_us);
        last_time_us = x.time_us;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
        }
    };

    auto fast_dataf = [](const sensor_data &x, bool catchup) {};

    auto q = setup_queue(dataf, fast_dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);

    q->start_buffering(std::chrono::microseconds(buffer_time_us));

    uint64_t time_us;
    int packets = 0;
    for(time_us = 0; time_us <= start_time_us; time_us += 2000) {
        q->receive_sensor_data(std::move(gyro_for_time(time_us)));
        q->receive_sensor_data(std::move(accel_for_time(time_us)));
        q->receive_sensor_data(std::move(gray8_for_time(time_us)));
        q->receive_sensor_data(std::move(depth16_for_time(time_us)));
        if(time_us >= start_time_us - buffer_time_us) {
            packets++;
        }
    }

    // give the buffer time to catch up
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    q->start(true);
    // give the queue time to start 
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    for(; time_us < start_time_us + extra_time_us; time_us += 2000) {
        q->receive_sensor_data(std::move(gyro_for_time(time_us)));
        q->receive_sensor_data(std::move(accel_for_time(time_us)));
        q->receive_sensor_data(std::move(gray8_for_time(time_us)));
        q->receive_sensor_data(std::move(depth16_for_time(time_us)));
        packets++;
    }

    q->stop();

    EXPECT_EQ(camrcv, packets);
    EXPECT_EQ(deprcv, packets);
    EXPECT_EQ(gyrrcv, packets);
    EXPECT_EQ(accrcv, packets);
}
