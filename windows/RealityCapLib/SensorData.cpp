//
//  sensor_data.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#define _USE_MATH_DEFINES
#include <math.h>
#include "stdafx.h"
#include "SensorData.h"
#include <stdexcept>
#include <iostream>
#include "rc_pxc_util.h"

camera_data camera_data_from_PXCImage(PXCImage *h)
{
    camera_data d;
    RCSavedImage *si = new RCSavedImage(h);
    d.image_handle = std::unique_ptr<void, void(*)(void *)>(si, RCSavedImage::releaseOpaquePointer);
    d.image = si->data.planes[0];
    d.stride = si->data.pitches[0];
    auto info = si->image->QueryInfo();
    d.width = info.width;
    d.height = info.height;
    //Subtract off 6370 (blank interval) and half of frame time. Todo: get actual exposure time
    d.timestamp = sensor_clock::ns100_to_tp(si->image->QueryTimeStamp() - 6370 - 166670);
    return std::move(d);
}

accelerometer_data accelerometer_data_from_imu_sample_t(const imu_sample_t *sample)
{
    accelerometer_data data;
    rc_Vector a = rc_convertAcceleration(sample);
    data.accel_m__s2[0] = a.x;
    data.accel_m__s2[1] = a.y;
    data.accel_m__s2[2] = a.z;
    data.timestamp = sensor_clock::ns100_to_tp(sample->coordinatedUniversalTime100ns);
    return data;
}

gyro_data gyro_data_from_imu_sample_t(const imu_sample_t *sample)
{
    gyro_data data;
    rc_Vector a = rc_convertAngularVelocity(sample);
    data.angvel_rad__s[0] = a.x;
    data.angvel_rad__s[1] = a.y;
    data.angvel_rad__s[2] = a.z;
    data.timestamp = sensor_clock::ns100_to_tp(sample->coordinatedUniversalTime100ns);
    return data;
}