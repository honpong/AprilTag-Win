//
//  sensor_data.h
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__sensor_data__
#define __RC3DK__sensor_data__

#include <memory>
#include "platform/sensor_clock.h"

class camera_data
{
public:
    camera_data(): image_handle(nullptr, nullptr), image(nullptr), width(0), height(0), stride(0) { }
    camera_data(camera_data&& other) = default;
    camera_data &operator=(camera_data&& other) = default;
    
    sensor_clock::time_point timestamp;
    std::unique_ptr<void, void(*)(void *)> image_handle;
    uint8_t *image;
    int width, height, stride;
};

class accelerometer_data
{
public:
    sensor_clock::time_point timestamp;
    float accel_m__s2[3];
    accelerometer_data() {};
    
    accelerometer_data(const accelerometer_data&) = delete;
    accelerometer_data& operator=(const accelerometer_data&) = delete;
    
    accelerometer_data(accelerometer_data&& other) = default;
    accelerometer_data &operator=(accelerometer_data&& other) = default;
};

class gyro_data
{
public:
    sensor_clock::time_point timestamp;
    float angvel_rad__s[3];
    gyro_data() {}
    
    gyro_data(const gyro_data&) = delete;
    gyro_data& operator=(const gyro_data&) = delete;
    
    gyro_data(gyro_data&& other) = default;
    gyro_data &operator=(gyro_data&& other) = default;
};

#endif /* defined(__RC3DK__sensor_data__) */
