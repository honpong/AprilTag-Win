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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "platform/sensor_clock.h"
#include "../filter/rc_tracker.h"
#include <algorithm>
#include "../cor/sensor.h"

template<int size_>
class sensor_data
{
public:
    sensor_clock::time_point timestamp;
    sensor_storage<size_> *source;
};

template<rc_ImageFormat camera_type, class data_type, int size_>
class image_data: public sensor_data<size_>
{
public:
    image_data(): image_handle(nullptr, nullptr), image(nullptr), width(0), height(0), stride(0) { }
    image_data(image_data<camera_type, data_type, size_>&& other) = default;
    image_data &operator=(image_data<camera_type, data_type, size_>&& other) = default;
    
    image_data(int image_width, int image_height) :
    image_handle(malloc(image_width * image_height * sizeof(data_type)), free), image((data_type *)image_handle.get()), width(image_width), height(image_height), stride(image_width * sizeof(data_type))
    {
        assert(height && stride);
    }
    
    image_data(int image_width, int image_height, data_type initial_value): image_data(image_width, image_height)
    {
        std::fill_n(image, width * height, initial_value);
    }
    
    image_data<camera_type, data_type, size_> make_copy() const
    {
        assert(height && stride);
        image_data<camera_type, data_type, size_> res;
        res.source = this->source;
        res.timestamp = this->timestamp;
        res.width = width;
        res.height = height;
        res.stride = stride;
        res.image = (data_type *)malloc(stride*height);
        if(height && stride)
        {
            res.image_handle = std::unique_ptr<void, void(*)(void *)>(res.image, free);
            memcpy(res.image, image, stride*height);
        }
        return res;
    }
    
    image_data(const image_data<camera_type, data_type, size_> &other) = delete;
    image_data &operator=(const image_data<camera_type, data_type, size_>& other) = delete;

    sensor_clock::duration exposure_time;
    std::unique_ptr<void, void(*)(void *)> image_handle;
    data_type *image;
    int width, height, stride;
};

typedef image_data<rc_FORMAT_GRAY8, uint8_t, 2> image_gray8;
typedef image_data<rc_FORMAT_DEPTH16, uint16_t, 1> image_depth16;

class accelerometer_data: public sensor_data<3>
{
public:
    float accel_m__s2[3];
    accelerometer_data() {};
    
    accelerometer_data(const accelerometer_data& other) = default;
    accelerometer_data &operator=(const accelerometer_data&) = delete;
    
    accelerometer_data(accelerometer_data&& other) = default;
    accelerometer_data &operator=(accelerometer_data&& other) = default;
};

class gyro_data: public sensor_data<3>
{
public:
    float angvel_rad__s[3];
    gyro_data() {}
    
    gyro_data(const gyro_data& other) = default;
    gyro_data &operator=(const gyro_data&) = delete;
    
    gyro_data(gyro_data&& other) = default;
    gyro_data &operator=(gyro_data&& other) = default;
};

#endif /* defined(__RC3DK__sensor_data__) */
