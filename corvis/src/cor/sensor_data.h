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

class sensor_data
{
public:
    const sensor *source;
    sensor_clock::time_point timestamp;
};

template<rc_ImageFormat camera_type, class data_type>
class image_data: public sensor_data
{
public:
    image_data(): image_handle(nullptr, nullptr), image(nullptr), width(0), height(0), stride(0) { }
    image_data(image_data<camera_type, data_type>&& other) = default;
    image_data &operator=(image_data<camera_type, data_type>&& other) = default;
    image_data(int image_width, int image_height, int image_stride) :
        image_handle(nullptr, nullptr), image(nullptr), width(image_width), height(image_height), stride(image_stride) {
            assert(stride % sizeof(data_type) == 0);
            if(height && stride) {
                image = (data_type *)malloc(stride*height);
                image_handle = std::unique_ptr<void, void(*)(void *)>(image, [](void * image_ptr) {
                            free(image_ptr);
                        });
            }
    }
    image_data(int image_width, int image_height, int image_stride, data_type initial_value) :
        image_data(image_width, image_height, image_stride) {
            for (int y=0; y<height; y++)
                std::fill_n(image + y*stride/sizeof(data_type), width, initial_value);
    }
    image_data(const image_data<camera_type, data_type> &other) :
        image_data(other.width, other.height, other.stride) {
            timestamp = other.timestamp;
            exposure_time = other.exposure_time;
            if(height && stride) {
                memcpy(image, other.image, stride*height);
            }
    }
    image_data &operator=(const image_data<camera_type, data_type>& other) = delete;

    sensor_clock::duration exposure_time;
    std::unique_ptr<void, void(*)(void *)> image_handle;
    data_type *image;
    int width, height, stride;
};

typedef image_data<rc_FORMAT_GRAY8, uint8_t> image_gray8;
typedef image_data<rc_FORMAT_DEPTH16, uint16_t> image_depth16;

class accelerometer_data: public sensor_data
{
public:
    float accel_m__s2[3];
    accelerometer_data() {};
    
    accelerometer_data(const accelerometer_data& other) = default;
    accelerometer_data &operator=(const accelerometer_data&) = delete;
    
    accelerometer_data(accelerometer_data&& other) = default;
    accelerometer_data &operator=(accelerometer_data&& other) = default;
};

class gyro_data: public sensor_data
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
