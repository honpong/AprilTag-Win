#include "gtest/gtest.h"
#include "sensor_fusion_queue.h"
#include "util.h"

std::unique_ptr<fusion_queue> setup_queue(std::function<void(sensor_data && x)> dataf, fusion_queue::latency_strategy strategy, uint64_t max_latency_us)
{
    std::unique_ptr<fusion_queue> q = std::make_unique<fusion_queue>(dataf, strategy, max_latency_us);
    q->require_sensor(rc_SENSOR_TYPE_IMAGE, 0, 0);
    q->require_sensor(rc_SENSOR_TYPE_DEPTH, 0, 0);
    q->require_sensor(rc_SENSOR_TYPE_ACCELEROMETER, 0, 0);
    q->require_sensor(rc_SENSOR_TYPE_GYROSCOPE, 0, 0);

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

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);
    
    q->start_sync();

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
    
    auto dataf = [&last_cam_time, &last_dep_time, &last_acc_time, &last_gyr_time, &camrcv, &deprcv, &accrcv, &gyrrcv](sensor_data && x) {
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE:
                EXPECT_GE(x.time_us, last_cam_time);
                last_cam_time = x.time_us;
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

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, jitter_us);

    auto start = sensor_clock::now();
    
    q->start_async();
    
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

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);
    
    q->start_sync();
    
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

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_LATENCY, 5000);
    
    q->start_sync();
    
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

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);

    q->start_sync();

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

TEST(SensorFusionQueue, FIFO)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    uint64_t times[] = {2000, 3000, 1000, 2000, 3000, 4000, 800};
    int i = 0;

    auto dataf = [&times, &i, &camrcv, &deprcv, &accrcv, &gyrrcv](sensor_data && x) {
        EXPECT_EQ(x.time_us, times[i]);
        i++;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
        }
    };

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::FIFO, 5000);

    q->start_singlethreaded();

    q->receive_sensor_data(std::move(gyro_for_time(times[0])));

    q->receive_sensor_data(std::move(accel_for_time(times[1])));

    q->receive_sensor_data(std::move(gray8_for_time(times[2])));

    q->receive_sensor_data(std::move(gray8_for_time(times[3])));

    q->receive_sensor_data(std::move(gray8_for_time(times[4])));

    q->receive_sensor_data(std::move(gray8_for_time(times[5])));

    q->receive_sensor_data(std::move(depth16_for_time(times[6])));

    q->stop();

    EXPECT_EQ(camrcv, 4);
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

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::ELIMINATE_DROPS, 5000);

    q->start_sync();

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
