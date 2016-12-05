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
#include "quaternion.h"
#include "transformation.h"
#include "rc_tracker.h"

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
    sensor(int _id, std::string _name = "") : id(_id), name(_name) {}
};

template<int size_> class sensor_storage : public sensor
{
public:
    stdev<size_> meas_stdev, inn_stdev, stability;
    v<size_> start_variance;
    f_t measurement_variance;
    using sensor::sensor;
    void init_with_variance(f_t variance) {
        got = false;
        meas_stdev = stdev<size_>();
        inn_stdev = stdev<size_>();
        stability = stdev<size_>();
        measurement_variance = variance;
    }
};

struct sensor_camera {
    struct rc_CameraIntrinsics intrinsics = {};
};

struct sensor_grey : public sensor_storage<2>, public sensor_camera {
    using sensor_storage<2>::sensor_storage;
};
struct sensor_depth : public sensor_storage<1>, public sensor_camera {
    using sensor_storage<1>::sensor_storage;
};
struct sensor_accelerometer : public sensor_storage<3> {
    rc_AccelerometerIntrinsics intrinsics;
    using sensor_storage<3>::sensor_storage;
};
struct sensor_gyroscope : public sensor_storage<3> {
    rc_GyroscopeIntrinsics intrinsics;
    using sensor_storage<3>::sensor_storage;
};

#endif /* sensor_h */