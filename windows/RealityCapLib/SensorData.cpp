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

typedef std::pair<PXCImage *, PXCImage::ImageData> handle_type;

static void cleanup_handle(void *h)
{
    auto handle = (handle_type *)h;
    if (handle)
    {
        auto res = handle->first->ReleaseAccess(&handle->second);
        handle->first->Release();
    }
    delete handle;
}

camera_data camera_data_from_PXCImage(PXCImage *h)
{
    camera_data d;
    d.image_handle = std::unique_ptr<void, void(*)(void *)>(new handle_type, cleanup_handle);
    auto handle = (handle_type *)d.image_handle.get();
    handle->first = (PXCImage *)h;
    auto result = handle->first->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_Y8, &handle->second);
    if (result != PXC_STATUS_NO_ERROR || !handle->second.planes[0]) throw std::runtime_error("PXCImage->AcquireAccess failed!"); 
    d.image = handle->second.planes[0];
    d.stride = handle->second.pitches[0];
    auto info = handle->first->QueryInfo();
    d.width = info.width;
    d.height = info.height;
    //Subtract off 6370 (blank interval) and half of frame time. Todo: get actual exposure time
    d.timestamp = sensor_clock::ns100_to_tp(handle->first->QueryTimeStamp() - 6370 - 166670);
    handle->first->AddRef();
    return std::move(d);
}

accelerometer_data accelerometer_data_from_imu_sample_t(const imu_sample_t *sample)
{
    accelerometer_data data;
    //windows gives acceleration in g-units, so multiply by standard gravity in m/s^2
    data.accel_m__s2[0] = -sample->data[1] * 9.80665;
    data.accel_m__s2[1] = sample->data[0] * 9.80665;
    data.accel_m__s2[2] = -sample->data[2] * 9.80665;
    data.timestamp = sensor_clock::ns100_to_tp(sample->coordinatedUniversalTime100ns);
    return data;
}

gyro_data gyro_data_from_imu_sample_t(const imu_sample_t *sample)
{
    gyro_data data;
    //windows gives angular velocity in degrees per second
    data.angvel_rad__s[0] = sample->data[0] * M_PI / 180.;
    data.angvel_rad__s[1] = sample->data[1] * M_PI / 180.;
    data.angvel_rad__s[2] = sample->data[2] * M_PI / 180.;
    data.timestamp = sensor_clock::ns100_to_tp(sample->coordinatedUniversalTime100ns);
    return data;
}