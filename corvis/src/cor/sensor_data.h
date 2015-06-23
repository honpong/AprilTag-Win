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

enum class camera_enum
{
    gray,
    rgb,
    depth
};

template<camera_enum camera_type, class data_type>
class image_data
{
public:
    image_data(): image_handle(nullptr, nullptr), image(nullptr), width(0), height(0), stride(0) { }
    image_data(image_data<camera_type, data_type>&& other) = default;
    image_data &operator=(image_data<camera_type, data_type>&& other) = default;
    
    sensor_clock::time_point timestamp;
    std::unique_ptr<void, void(*)(void *)> image_handle;
    data_type *image;
    int width, height, stride;
};

typedef image_data<camera_enum::gray, uint8_t> image_gray8;
typedef image_data<camera_enum::depth, uint16_t> image_depth16;

class camera_data: public image_data<camera_enum::gray, uint8_t>
{
public:
    camera_data(): image_data(), depth(nullptr) { }
    camera_data(image_gray8 &&other): image_gray8(std::move(other)) {}

    std::unique_ptr<image_depth16> depth;
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
