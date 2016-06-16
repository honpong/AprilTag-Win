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
};

template<int size_>
struct sensor_storage : sensor {
    stdev<size_> meas_stdev, inn_stdev;
    void init() {
        meas_stdev = stdev<size_>();
        inn_stdev = stdev<size_>();
    }
};

#endif /* sensor_h */
