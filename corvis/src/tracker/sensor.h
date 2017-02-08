//
//  sensor.h
//  RC3DK
//
//  Created by Eagle Jones on 4/3/16.
//  Copyright © 2016 Intel. All rights reserved.
//

#ifndef sensor_h
#define sensor_h

#include "vec4.h"
#include "stdev.h"
#include "quaternion.h"
#include "transformation.h"
#include "rc_tracker.h"

#include <chrono>

struct sensor
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
    const int id;
    const std::string name;
    bool got = false;
    struct extrinsics {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
        transformation mean;
        struct transformation_variance { v3 Q = v3::Zero(), T = v3::Zero(); } variance;
    } extrinsics;
    sensor(int _id, std::string _name) : id(_id), name(_name) {}
    sensor(int _id) : id(_id), name("Sensor" + std::to_string(_id)) {}
};

template<int size_> class sensor_storage : public sensor
{
public:
    stdev<size_> meas_stdev, inn_stdev, stability;
    v<size_> start_variance;
    v<size_> last_meas;
    f_t measurement_variance;
    stdev<1> measure_time_stats;
    stdev<1> other_time_stats;
    using sensor::sensor;
    void init_with_variance(f_t variance) {
        got = false;
        meas_stdev = stdev<size_>();
        inn_stdev = stdev<size_>();
        stability = stdev<size_>();
        last_meas = v<size_>::Zero();
        measure_time_stats = decltype(measure_time_stats) {};
        measurement_variance = variance;
    }
};

struct sensor_camera {
    struct rc_CameraIntrinsics intrinsics = {};
};

struct sensor_grey : public sensor_storage<2>, public sensor_camera {
    using sensor_storage<2>::sensor_storage;
    sensor_grey(int _id) : sensor_storage<2>(_id, "Camera" + std::to_string(_id)) {}
};
struct sensor_depth : public sensor_storage<1>, public sensor_camera {
    using sensor_storage<1>::sensor_storage;
    sensor_depth(int _id) : sensor_storage<1>(_id, "Depth" + std::to_string(_id)) {}
};
struct sensor_accelerometer : public sensor_storage<3> {
    rc_AccelerometerIntrinsics intrinsics;
    using sensor_storage<3>::sensor_storage;
    sensor_accelerometer(int _id) : sensor_storage<3>(_id, "Accel" + std::to_string(_id)) {}
};
struct sensor_gyroscope : public sensor_storage<3> {
    rc_GyroscopeIntrinsics intrinsics;
    using sensor_storage<3>::sensor_storage;
    sensor_gyroscope(int _id) : sensor_storage<3>(_id, "Gyro" + std::to_string(_id)) {}
};

#endif /* sensor_h */
