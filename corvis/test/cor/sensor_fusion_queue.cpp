#include "gtest/gtest.h"
#include "sensor_fusion_queue.h"
#include "util.h"

TEST(SensorFusionQueue, Reorder)
{
    sensor_clock::time_point last_time;
    sensor_clock::time_point this_gyro;
    sensor_clock::time_point this_accel;
    auto camf = [&last_time](image_gray8 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto depthf = [&last_time](image_depth16 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto accf = [&last_time](accelerometer_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto gyrf = [&last_time](gyro_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto fast_accf = [&this_accel](const accelerometer_data &x) { EXPECT_EQ(x.timestamp, this_accel); };
    auto fast_gyrf = [&this_gyro](const gyro_data &x) { EXPECT_GE(x.timestamp, this_gyro); };
    
    std::unique_ptr<fusion_queue> q = std::make_unique<fusion_queue>(camf, depthf, accf, gyrf, fast_accf, fast_gyrf, fusion_queue::latency_strategy::BALANCED, std::chrono::microseconds(5000));
    
    q->start_sync(true);

    gyro_data g;
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(0));
    this_gyro = g.timestamp;
    q->receive_gyro(std::move(g));

    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(10000));
    this_gyro = g.timestamp;
    q->receive_gyro(std::move(g));

    accelerometer_data a;
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(8000));
    this_accel = a.timestamp;
    q->receive_accelerometer(std::move(a));

    image_depth16 d;
    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q->receive_depth(std::move(d));

    image_gray8 c;
    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q->receive_camera(std::move(c));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(18000));
    this_accel = a.timestamp;
    q->receive_accelerometer(std::move(a));

    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(20000));
    this_gyro = g.timestamp;
    q->receive_gyro(std::move(g));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(30000));
    this_gyro = g.timestamp;
    q->receive_gyro(std::move(g));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(28000));
    this_accel = a.timestamp;
    q->receive_accelerometer(std::move(a));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    this_accel = a.timestamp;
    q->receive_accelerometer(std::move(a));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(40000));
    this_gyro = g.timestamp;
    q->receive_gyro(std::move(g));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(48000));
    this_accel = a.timestamp;
    q->receive_accelerometer(std::move(a));
    
    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q->receive_depth(std::move(d));

    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q->receive_camera(std::move(c));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(50000));
    this_gyro = g.timestamp;
    q->receive_gyro(std::move(g));
    
    q->stop_sync();
}

TEST(SensorFusionQueue, Threading)
{
    sensor_clock::time_point last_cam_time;
    sensor_clock::time_point last_dep_time;
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
    int depsent = 0;
    int gyrsent = 0;
    int accsent = 0;
    
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    
    int fast_accrcv = 0;
    int fast_gyrrcv = 0;
    
    auto camf = [&last_cam_time, &camrcv](image_gray8 &&x) { EXPECT_GE(x.timestamp, last_cam_time); last_cam_time = x.timestamp; ++camrcv; };
    auto depthf = [&last_dep_time, &deprcv](image_depth16 &&x) { EXPECT_GE(x.timestamp, last_dep_time); last_dep_time = x.timestamp; ++deprcv; };
    auto accf = [&last_acc_time, &accrcv](accelerometer_data &&x) { EXPECT_GE(x.timestamp, last_acc_time); last_acc_time = x.timestamp; ++accrcv; };
    auto gyrf = [&last_gyr_time, &gyrrcv](gyro_data &&x) { EXPECT_GE(x.timestamp, last_gyr_time); last_gyr_time = x.timestamp; ++gyrrcv; };
    auto fast_accf = [&fast_accrcv](const accelerometer_data &x) { ++fast_accrcv; };
    auto fast_gyrf = [&fast_gyrrcv](const gyro_data &x) { ++fast_gyrrcv; };

    std::unique_ptr<fusion_queue> q = std::make_unique<fusion_queue>(camf, depthf, accf, gyrf, fast_accf, fast_gyrf, fusion_queue::latency_strategy::BALANCED, jitter);

    auto start = sensor_clock::now();
    
    q->start_sync(true);
    
    std::thread camthread([&q, start, camera_interval, cam_latency, &camsent, thread_time]{
        image_gray8 x;
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            x.timestamp = sensor_clock::time_point(duration);
            std::this_thread::sleep_for(cam_latency);
            q->receive_camera(std::move(x));
            ++camsent;
            std::this_thread::sleep_for(camera_interval-cam_latency);
        }
    });
    std::thread depthread([&q, start, camera_interval, cam_latency, &camsent, thread_time]{
        image_depth16 x;
        auto now = sensor_clock::now();
        auto start_time = now;
        while((now = sensor_clock::now()) < start_time + thread_time)
        {
            auto duration = now - start;
            x.timestamp = sensor_clock::time_point(duration);
            std::this_thread::sleep_for(cam_latency);
            q->receive_depth(std::move(x));
            ++camsent;
            std::this_thread::sleep_for(camera_interval-cam_latency);
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
            q->receive_gyro(std::move(x));
            ++gyrsent;
            std::this_thread::sleep_for(inertial_interval-in_latency);
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
            q->receive_accelerometer(std::move(x));
            ++accsent;
            std::this_thread::sleep_for(inertial_interval-in_latency);
        }
    });
    
    camthread.join();
    depthread.join();
    gyrothread.join();
    accelthread.join();
    q->stop_sync();
    EXPECT_EQ(fast_accrcv, accsent);
    EXPECT_EQ(fast_gyrrcv, gyrsent);
}

TEST(ThreadedDispatch, DropLate)
{
    sensor_clock::time_point last_time;
    auto camf = [&last_time](image_gray8 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto depthf = [&last_time](image_depth16 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto accf = [&last_time](accelerometer_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; EXPECT_NE(x.timestamp, sensor_clock::time_point(std::chrono::microseconds(17000))); };
    auto gyrf = [&last_time](gyro_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; EXPECT_NE(x.timestamp, sensor_clock::time_point(std::chrono::microseconds(30000))); };
    auto fast_accf = [&last_time](const accelerometer_data &x) { };
    auto fast_gyrf = [&last_time](const gyro_data &x) { };
    
    std::unique_ptr<fusion_queue> q = std::make_unique<fusion_queue>(camf, depthf, accf, gyrf, fast_accf, fast_gyrf, fusion_queue::latency_strategy::BALANCED, std::chrono::microseconds(5000));
    
    q->start_sync(true);
    
    gyro_data g;
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(0));
    q->receive_gyro(std::move(g));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(10000));
    q->receive_gyro(std::move(g));
    
    accelerometer_data a;
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(8000));
    q->receive_accelerometer(std::move(a));
    
    image_gray8 c;
    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q->receive_camera(std::move(c));

    image_depth16 d;
    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q->receive_depth(std::move(d));

    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(18000));
    q->receive_accelerometer(std::move(a));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(20000));
    q->receive_gyro(std::move(g));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(19000));
    q->receive_accelerometer(std::move(a));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(28000));
    q->receive_accelerometer(std::move(a));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q->receive_accelerometer(std::move(a));
    
    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q->receive_camera(std::move(c));

    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(38000));
    q->receive_depth(std::move(d));
    
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(48000));
    q->receive_accelerometer(std::move(a));
    
    //NOTE: This makes this test a little non-deterministic - if it fails due to the 30000 timestamp showing up this could be why
    //If we remove the sleep, everything gets into the queue before the dispatch thread even starts, so everything shows up on the other side in order
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(30000));
    q->receive_gyro(std::move(g));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(40000));
    q->receive_gyro(std::move(g));
    
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(50000));
    q->receive_gyro(std::move(g));
    
    q->stop_sync();
}

TEST(SensorFusionQueue, SameTime)
{
    int camrcv = 0;
    int deprcv = 0;
    int gyrrcv = 0;
    int accrcv = 0;
    sensor_clock::time_point last_time;

    auto camf = [&last_time, &camrcv](image_gray8 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; ++camrcv; };
    auto depthf = [&last_time, &deprcv](image_depth16 &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; ++deprcv; };
    auto accf = [&last_time, &accrcv](accelerometer_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; ++accrcv; };
    auto gyrf = [&last_time, &gyrrcv](gyro_data &&x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; ++gyrrcv; };
    auto fast_accf = [](const accelerometer_data &x) { };
    auto fast_gyrf = [](const gyro_data &x) { };

    std::unique_ptr<fusion_queue> q = std::make_unique<fusion_queue>(camf, depthf, accf, gyrf, fast_accf, fast_gyrf, fusion_queue::latency_strategy::BALANCED, std::chrono::microseconds(5000));

    q->start_sync(true);

    gyro_data g;
    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q->receive_gyro(std::move(g));

    accelerometer_data a;
    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q->receive_accelerometer(std::move(a));

    image_depth16 d;
    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q->receive_depth(std::move(d));

    image_gray8 c;
    c.timestamp = sensor_clock::time_point(std::chrono::microseconds(5000));
    q->receive_camera(std::move(c));

    q->stop_sync();

    EXPECT_EQ(camrcv, 1);
    EXPECT_EQ(deprcv, 1);
    EXPECT_EQ(gyrrcv, 1);
    EXPECT_EQ(accrcv, 1);
}

TEST(SensorQueue, Iterate)
{
    std::mutex mx;
    std::condition_variable cnd;
    bool active = true;
    accelerometer_data x;
    const accelerometer_data *d = nullptr;
    sensor_queue<accelerometer_data, 10> queue(mx, cnd, active);

    std::unique_lock<std::mutex> lock(mx);
    EXPECT_EQ(queue.peek_next(d, lock), nullptr);
    lock.unlock();
    for(int i = 0; i < 10; ++i)
    {
        x.timestamp = sensor_clock::time_point(std::chrono::microseconds(i));
        queue.push(std::move(x));
    }
    lock.lock();
    for(int i = 0; i < 10; ++i)
    {
        d = queue.peek_next(d, lock);
        EXPECT_EQ(d->timestamp.time_since_epoch().count(), i);
    }
    EXPECT_EQ(d = queue.peek_next(d, lock), nullptr);
    
    EXPECT_EQ(queue.pop(lock).timestamp.time_since_epoch().count(), 0);
    EXPECT_EQ(queue.pop(lock).timestamp.time_since_epoch().count(), 1);
    lock.unlock();
    x.timestamp = sensor_clock::time_point(std::chrono::microseconds(10));
    queue.push(std::move(x));
    x.timestamp = sensor_clock::time_point(std::chrono::microseconds(11));
    queue.push(std::move(x));
    
    lock.lock();
    for(int i = 2; i < 12; ++i)
    {
        d = queue.peek_next(d, lock);
        EXPECT_EQ(d->timestamp.time_since_epoch().count(), i);
    }
    EXPECT_EQ(queue.peek_next(d, lock), nullptr);
    for(int i = 2; i < 12; ++i)
    {
        EXPECT_EQ(queue.pop(lock).timestamp.time_since_epoch().count(), i);
    }
    lock.unlock();
}
