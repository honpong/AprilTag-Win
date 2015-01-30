#include "gtest/gtest.h"
#include "threaded_dispatch.h"
#include "util.h"

TEST(ThreadedDispatch, None)
{
    uint64_t last_time = 0;
    auto camf = [&last_time](const camera_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto accf = [&last_time](const accelerometer_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    auto gyrf = [&last_time](const gyro_data &x) { EXPECT_GE(x.timestamp, last_time); last_time = x.timestamp; };
    
    fusion_queue q(camf, accf, gyrf, 33000, 10000, 5000);
    struct gyro_data g;
    g.timestamp = 0;
    q.receive_gyro(g);

    g.timestamp = 10000;
    q.receive_gyro(g);

    struct accelerometer_data a;
    a.timestamp = 8000;
    q.receive_accelerometer(a);

    struct camera_data c;
    c.timestamp = 5000;
    q.receive_camera(c);

    a.timestamp = 18000;
    q.receive_accelerometer(a);

    g.timestamp = 20000;
    q.receive_gyro(g);
    
    g.timestamp = 30000;
    q.receive_gyro(g);

    a.timestamp = 28000;
    q.receive_accelerometer(a);

    a.timestamp = 38000;
    q.receive_accelerometer(a);
    
    g.timestamp = 40000;
    q.receive_gyro(g);

    a.timestamp = 48000;
    q.receive_accelerometer(a);
    
    c.timestamp = 38000;
    q.receive_camera(c);
    
    g.timestamp = 50000;
    q.receive_gyro(g);
    
    q.dispatch();
    EXPECT_EQ(1,1);
}
