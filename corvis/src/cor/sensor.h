//
//  sensor.h
//  RC3DK
//
//  Created by Eagle Jones on 4/3/16.
//  Copyright © 2016 Intel. All rights reserved.
//

#ifndef sensor_h
#define sensor_h

#include "../numerics/vec4.h"
#include "../numerics/quaternion.h"
#include "../numerics/transformation.h"

class sensor
{
public:
    const int id;
    const std::string name;
    sensor(int _id, std::string _name = "") : id(_id), name(_name) {}
    transformation extrinsics;
};

template<int size_> class sensor_storage : public sensor
{
public:
    stdev<size_> meas_stdev, inn_stdev;
    sensor_storage(int id, std::string name = "") : sensor(id, name) {}
    void init() {
        meas_stdev = stdev<size_>();
        inn_stdev = stdev<size_>();
    }
};

typedef sensor_storage<2> sensor_camera;
typedef sensor_storage<1> sensor_depth;
typedef sensor_storage<3> sensor_accelerometer;
typedef sensor_storage<3> sensor_gyroscope;

#endif /* sensor_h */
