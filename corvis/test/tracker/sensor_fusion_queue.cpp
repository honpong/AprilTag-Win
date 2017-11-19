#include "gtest/gtest.h"
#include "sensor_fusion_queue.h"
#include "util.h"

std::unique_ptr<fusion_queue> setup_queue(std::function<void(sensor_data && x)> dataf, fusion_queue::latency_strategy strategy, uint64_t max_latency_us)
{
    std::unique_ptr<fusion_queue> q = std::make_unique<fusion_queue>(dataf, strategy, std::chrono::microseconds(max_latency_us));
    q->require_sensor(rc_SENSOR_TYPE_IMAGE, 0, std::chrono::microseconds(0));
    q->require_sensor(rc_SENSOR_TYPE_DEPTH, 0, std::chrono::microseconds(0));
    q->require_sensor(rc_SENSOR_TYPE_ACCELEROMETER, 0, std::chrono::microseconds(0));
    q->require_sensor(rc_SENSOR_TYPE_GYROSCOPE, 0, std::chrono::microseconds(0));

    return q;
}

sensor_data depth16_for_time(uint64_t timestamp_us)
{
    std::unique_ptr<void, void(*)(void *)> nothing{nullptr, nullptr};
    sensor_data d(timestamp_us, rc_SENSOR_TYPE_DEPTH, 0 /* id */,
                0 /*rc_Timestamp shutter_time_us */,
                0, 0, 0, /*int width, int height, int stride,*/
                rc_FORMAT_DEPTH16, nullptr, std::move(nothing));

    return d;
}

sensor_data gray8_for_time(uint64_t timestamp_us)
{
    std::unique_ptr<void, void(*)(void *)> nothing{nullptr, nullptr};
    sensor_data d(timestamp_us, rc_SENSOR_TYPE_IMAGE, 0 /* id */,
                0 /*rc_Timestamp shutter_time_us */,
                0, 0, 0, /*int width, int height, int stride,*/
                rc_FORMAT_GRAY8, nullptr, std::move(nothing));
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

sensor_data temp_for_time(uint64_t timestamp_us)
{
    float t = {};
    sensor_data d(timestamp_us, rc_SENSOR_TYPE_THERMOMETER, 0 /* id */, t);
    return d;
}

class intv
{
public:
    int x;
    intv(): x(0) {};
    intv(int _x): x(_x) {};
    bool operator==(const intv &other) const { return x == other.x; }
    intv operator++() { ++x; return *this; }
    intv operator--() { --x; return *this; }
    bool operator<(const intv &other) const { return x < other.x; }
    bool operator>=(const intv &other) const { return x >= other.x; }
};

TEST(SensorFusionQueue, RingBuffer)
{
    sorted_ring_buffer<intv, 10> buf;
    
    intv in {0}, out {0}, x;
    for(in = 0, out = 0; in < 100; ++in, ++out)
    {
        buf.push(intv(in));
        EXPECT_EQ(in, buf.pop());
    }
    
    for(in = 0; in < 10; ++in) buf.push(intv(in));
    EXPECT_TRUE(buf.full());
    for(out = 0; out < 10; ++out) EXPECT_EQ(out, buf.pop());
    EXPECT_TRUE(buf.empty());

    for(; in < 20; ++in) buf.push(intv(in));
    EXPECT_TRUE(buf.full());
    for(; out < 20; ++out) EXPECT_EQ(out, buf.pop());
    EXPECT_TRUE(buf.empty());

    for(in = 30-1; in >= 20; --in) buf.push(intv(in));
    EXPECT_TRUE(buf.full());
    for(out = 20; out < 30; ++out) EXPECT_EQ(out, buf.pop());
    EXPECT_TRUE(buf.empty());

    for(in = 30; in < 40; ++in) buf.push(intv(in));
    EXPECT_TRUE(buf.full());
    for(out = 30; out < 35; ++out) EXPECT_EQ(out, buf.pop());
    EXPECT_TRUE(!buf.empty());

    for(in = 45-1; in >= 40; --in) buf.push(intv(in));
    EXPECT_TRUE(buf.full());
    for(out = 35; out < 45; ++out) EXPECT_EQ(out, buf.pop());
    EXPECT_TRUE(buf.empty());

    for(in = 45; in < 55; ++in) buf.push(intv(in));
    EXPECT_TRUE(buf.full());
    for(out = 45; out < 55; ++out) EXPECT_EQ(out, buf.pop());
    EXPECT_TRUE(buf.empty());

    for (int i=0; i<1000; i+=7) {
        for(in = 100+7-1+i; in >= 100+0+i; --in) buf.push(intv(in));
        EXPECT_TRUE(!buf.full());
        for(out = 100+0+i; out < 100+7+i; ++out) EXPECT_EQ(out, buf.pop());
        EXPECT_TRUE(buf.empty());
    }
}

TEST(SensorFusionQueue, Reorder)
{
    uint64_t last_time = 0;
    auto dataf = [&last_time](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time);
        last_time = x.time_us;
    };

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_DROPS, 500000);

    q->start(true);

    q->receive_sensor_data(gyro_for_time(0));
    q->receive_sensor_data(accel_for_time(1));
    q->receive_sensor_data(gray8_for_time(2));
    q->receive_sensor_data(depth16_for_time(3));
    q->receive_sensor_data(gyro_for_time(10000));
    q->receive_sensor_data(accel_for_time(8000));
    q->receive_sensor_data(depth16_for_time(5000));
    q->receive_sensor_data(gray8_for_time(5000));
    q->receive_sensor_data(accel_for_time(18000));
    q->receive_sensor_data(gyro_for_time(20000));
    q->receive_sensor_data(gyro_for_time(30000));
    q->receive_sensor_data(accel_for_time(28000));
    q->receive_sensor_data(accel_for_time(38000));
    q->receive_sensor_data(gyro_for_time(40000));
    q->receive_sensor_data(accel_for_time(48000));
    q->receive_sensor_data(depth16_for_time(38000));
    q->receive_sensor_data(gray8_for_time(38000));
    q->receive_sensor_data(gyro_for_time(50000));

    q->stop();
    ASSERT_EQ(q->total_in, 18);
    ASSERT_EQ(q->total_out, 18);
}

#include <iostream>

TEST(SensorFusionQueue, FastCatchup)
{
    const uint64_t maximum_latency_us = 500000;

    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    int tmprcv = 0;
    int catchup_accrcv = 0;
    int catchup_gyrrcv = 0;
    
    std::unique_ptr<fusion_queue> q;
    
    auto dataf = [&catchup_accrcv, &catchup_gyrrcv, &camrcv, &deprcv, &accrcv, &gyrrcv, &tmprcv, &q](sensor_data && x) {
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE:
                q->dispatch_buffered([&catchup_accrcv, &catchup_gyrrcv](const sensor_data &x) {
                    switch(x.type) {
                        case rc_SENSOR_TYPE_ACCELEROMETER: ++catchup_accrcv; break;
                        case rc_SENSOR_TYPE_GYROSCOPE:     ++catchup_gyrrcv; break;
                        default: break;
                    }
                });
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
            case rc_SENSOR_TYPE_THERMOMETER:
                ++tmprcv;
                break;
            case rc_SENSOR_TYPE_STEREO:
            case rc_SENSOR_TYPE_DEBUG:
                break;
        }
    };

    q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_DROPS, maximum_latency_us);


    q->start(false);

    q->receive_sensor_data(gyro_for_time(0));

    EXPECT_EQ(1, gyrrcv);

    q->receive_sensor_data(depth16_for_time(5000));

    q->receive_sensor_data(gray8_for_time(5000));

    q->receive_sensor_data(accel_for_time(8000));

    EXPECT_EQ(0, deprcv);
    EXPECT_EQ(0, camrcv);
    EXPECT_EQ(0, accrcv);
    EXPECT_EQ(1, gyrrcv);
    EXPECT_EQ(0, tmprcv);
    EXPECT_EQ(0, catchup_accrcv);
    EXPECT_EQ(0, catchup_gyrrcv);

    q->receive_sensor_data(gyro_for_time(10000));

    EXPECT_EQ(1, deprcv);
    EXPECT_EQ(0, camrcv);
    EXPECT_EQ(0, accrcv);
    EXPECT_EQ(1, gyrrcv);
    EXPECT_EQ(0, tmprcv);
    EXPECT_EQ(0, catchup_accrcv);
    EXPECT_EQ(0, catchup_gyrrcv);

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

    q->receive_sensor_data(gray8_for_time(38000));

    EXPECT_EQ(2, deprcv);
    EXPECT_EQ(1, camrcv);
    EXPECT_EQ(4, accrcv);
    EXPECT_EQ(4, gyrrcv);
    EXPECT_EQ(5, catchup_accrcv);
    EXPECT_EQ(4, catchup_gyrrcv);

    q->receive_sensor_data(gyro_for_time(50000));

    q->stop();
}

TEST(SensorFusionQueue, Threading)
{
    uint64_t last_cam_time = 0;
    uint64_t last_dep_time = 0;
    uint64_t last_acc_time = 0;
    uint64_t last_gyr_time = 0;
    uint64_t last_tmp_time = 0;
    
    //Times in us
    //Thread time isn't really how long we'll spend since we just sleep for interval us
    auto thread_time = std::chrono::microseconds(100000);
    const sensor_clock::duration camera_interval = std::chrono::microseconds(66);
    const sensor_clock::duration inertial_interval = std::chrono::microseconds(20);
    const uint64_t maximum_latency_us = 10;
    const sensor_clock::duration cam_latency = std::chrono::microseconds(10);
    const sensor_clock::duration in_latency = std::chrono::microseconds(2);

    int camsent = 0;
    int gyrsent = 0;
    int accsent = 0;

    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    int tmprcv = 0;

    std::unique_ptr<fusion_queue> q;

    auto dataf = [&last_cam_time, &last_dep_time, &last_acc_time, &last_gyr_time, &last_tmp_time,
                  &camrcv, &deprcv, &accrcv, &gyrrcv, &tmprcv, &q](sensor_data && x) {
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE:
                EXPECT_GE(x.time_us, last_cam_time);
                last_cam_time = x.time_us;
                q->dispatch_buffered([](const sensor_data &) {});
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
            case rc_SENSOR_TYPE_THERMOMETER:
                EXPECT_GE(x.time_us, last_tmp_time);
                last_tmp_time = x.time_us;
                ++tmprcv;
                break;
            case rc_SENSOR_TYPE_STEREO:
            case rc_SENSOR_TYPE_DEBUG:
                break;
        }
    };
    
    q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_DROPS, maximum_latency_us);

    auto start = sensor_clock::now();
    
    q->start(true);
    
    std::thread camthread([&q, start, camera_interval, cam_latency, &camsent, thread_time]{
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            std::this_thread::sleep_for(cam_latency);
            q->receive_sensor_data(gray8_for_time(duration.count()));
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
            q->receive_sensor_data(depth16_for_time(duration.count()));
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
            q->receive_sensor_data(gyro_for_time(duration.count()));
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
            q->receive_sensor_data(accel_for_time(duration.count()));
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

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_DROPS, 5000);

    q->start(true);

    q->receive_sensor_data(gyro_for_time(0));
    q->receive_sensor_data(accel_for_time(8000));
    q->receive_sensor_data(gray8_for_time(5000));
    q->receive_sensor_data(depth16_for_time(5000));
    q->receive_sensor_data(accel_for_time(4000));

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

    q->start(true);

    q->receive_sensor_data(gyro_for_time(0));
    q->receive_sensor_data(gyro_for_time(10000));
    q->receive_sensor_data(accel_for_time(8000));
    q->receive_sensor_data(gray8_for_time(5000));
    q->receive_sensor_data(depth16_for_time(5000));
    q->receive_sensor_data(accel_for_time(18000));
    q->receive_sensor_data(gyro_for_time(20000));
    q->receive_sensor_data(accel_for_time(19000));
    q->receive_sensor_data(accel_for_time(28000));
    q->receive_sensor_data(accel_for_time(38000));
    q->receive_sensor_data(gray8_for_time(38000));
    q->receive_sensor_data(depth16_for_time(38000));
    q->receive_sensor_data(accel_for_time(48000));

    //NOTE: This makes this test a little non-deterministic - if it fails due to the 30000 timestamp showing up this could be why
    //If we remove the sleep, everything gets into the queue before the dispatch thread even starts, so everything shows up on the other side in order
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    q->receive_sensor_data(gyro_for_time(30000));

    q->receive_sensor_data(gyro_for_time(40000));

    q->receive_sensor_data(gyro_for_time(50000));

    q->stop();
}

TEST(SensorFusionQueue, SameTime)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    int tmprcv = 0;
    uint64_t last_time = 0;

    auto dataf = [&last_time, &camrcv, &deprcv, &accrcv, &gyrrcv, &tmprcv](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time);
        last_time = x.time_us;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
            case rc_SENSOR_TYPE_THERMOMETER: ++tmprcv; break;
            case rc_SENSOR_TYPE_STEREO: break;
            case rc_SENSOR_TYPE_DEBUG: break;
        }
    };

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_DROPS, 5000);

    q->start(true);

    q->receive_sensor_data(gyro_for_time(5000));
    q->receive_sensor_data(accel_for_time(5000));
    q->receive_sensor_data(gray8_for_time(5000));
    q->receive_sensor_data(temp_for_time(5000));
    q->receive_sensor_data(depth16_for_time(5000));

    q->stop();

    EXPECT_EQ(camrcv, 1);
    EXPECT_EQ(deprcv, 1);
    EXPECT_EQ(gyrrcv, 1);
    EXPECT_EQ(accrcv, 1);
    EXPECT_EQ(tmprcv, 1);
}

TEST(SensorFusionQueue, MaxLatencyDispatch)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    int tmprcv = 0;
    uint64_t last_time = 0;

    auto dataf = [&last_time, &camrcv, &deprcv, &accrcv, &gyrrcv, &tmprcv](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time);
        last_time = x.time_us;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
            case rc_SENSOR_TYPE_THERMOMETER: ++tmprcv; break;
            case rc_SENSOR_TYPE_STEREO: break;
            case rc_SENSOR_TYPE_DEBUG: break;
        }
    };
    
    auto q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_DROPS, 5000);

    q->start(true);

    q->receive_sensor_data(gyro_for_time(5000));
    q->receive_sensor_data(accel_for_time(5000));
    q->receive_sensor_data(gray8_for_time(5000));

    q->receive_sensor_data(gyro_for_time(6000));
    q->receive_sensor_data(gyro_for_time(7000));
    q->receive_sensor_data(gyro_for_time(8000));
    q->receive_sensor_data(gyro_for_time(9000));

    q->receive_sensor_data(accel_for_time(6000));
    q->receive_sensor_data(accel_for_time(7000));
    q->receive_sensor_data(accel_for_time(8000));
    q->receive_sensor_data(accel_for_time(9000));

    // we should dispatch here due to max latency of 5ms
    q->receive_sensor_data(gray8_for_time(10001));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // we should drop this, since we will have already dispatched t =
    // 5000
    q->receive_sensor_data(depth16_for_time(4000));

    q->stop();

    EXPECT_EQ(camrcv, 2);
    EXPECT_EQ(deprcv, 0);
    EXPECT_EQ(tmprcv, 0);
    EXPECT_EQ(gyrrcv, 5);
    EXPECT_EQ(accrcv, 5);
}

TEST(SensorFusionQueue, BufferNoDispatch)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    int tmprcv = 0;
    uint64_t buffer_time_us = 50000;
    uint64_t start_time_us = 1000000;
    uint64_t last_time_us = 0;

    auto dataf = [&last_time_us, &start_time_us, &buffer_time_us, &camrcv, &deprcv, &accrcv, &gyrrcv, &tmprcv](sensor_data && x) {
        EXPECT_GE(x.time_us, last_time_us);
        last_time_us = x.time_us;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
            case rc_SENSOR_TYPE_THERMOMETER: ++tmprcv; break;
            case rc_SENSOR_TYPE_STEREO: break;
            case rc_SENSOR_TYPE_DEBUG: break;
        }
    };

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_DROPS, 5000);

    q->start_buffering(std::chrono::microseconds(buffer_time_us));

    uint64_t time_us;
    for(time_us = 0; time_us < start_time_us; time_us += 2000) {
        q->receive_sensor_data(gyro_for_time(time_us));
        q->receive_sensor_data(accel_for_time(time_us));
        q->receive_sensor_data(gray8_for_time(time_us));
        q->receive_sensor_data(depth16_for_time(time_us));
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
    int tmprcv = 0;
    uint64_t buffer_time_us = 50'000; //'
    uint64_t start_time_us = 1'000'000;
    uint64_t last_time_us = 0;
    uint64_t extra_time_us = 100'000; //'

    auto dataf = [&last_time_us, &start_time_us, &buffer_time_us, &camrcv, &deprcv, &accrcv, &gyrrcv, &tmprcv](sensor_data && x) {
        EXPECT_GE(x.time_us, start_time_us - buffer_time_us);
        EXPECT_GE(x.time_us, last_time_us);
        last_time_us = x.time_us;
        switch(x.type) {
            case rc_SENSOR_TYPE_IMAGE: ++camrcv; break;
            case rc_SENSOR_TYPE_DEPTH: ++deprcv; break;
            case rc_SENSOR_TYPE_ACCELEROMETER: ++accrcv; break;
            case rc_SENSOR_TYPE_GYROSCOPE: ++gyrrcv; break;
            case rc_SENSOR_TYPE_THERMOMETER: ++tmprcv; break;
            case rc_SENSOR_TYPE_STEREO: break;
            case rc_SENSOR_TYPE_DEBUG: break;
        }
    };

    auto q = setup_queue(dataf, fusion_queue::latency_strategy::MINIMIZE_DROPS, 5000);

    q->start_buffering(std::chrono::microseconds(buffer_time_us));

    uint64_t time_us;
    int packets = 0;
    for(time_us = 0; time_us <= start_time_us; time_us += 2000) {
        q->receive_sensor_data(gyro_for_time(time_us));
        q->receive_sensor_data(accel_for_time(time_us));
        q->receive_sensor_data(gray8_for_time(time_us));
        q->receive_sensor_data(depth16_for_time(time_us));
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
        q->receive_sensor_data(gyro_for_time(time_us));
        q->receive_sensor_data(accel_for_time(time_us));
        q->receive_sensor_data(gray8_for_time(time_us));
        q->receive_sensor_data(depth16_for_time(time_us));
        packets++;
    }

    q->stop();

    EXPECT_EQ(camrcv, packets);
    EXPECT_EQ(deprcv, packets);
    EXPECT_EQ(gyrrcv, packets);
    EXPECT_EQ(accrcv, packets);
}
