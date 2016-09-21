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
#include <assert.h>
#include "../filter/rc_tracker.h"
#include "platform/sensor_clock.h"

class sensor_data : public rc_Data
{
private:
    std::unique_ptr<void, void(*)(void *)> image_handle{nullptr, nullptr};

public:
    sensor_clock::time_point timestamp;

    sensor_data(sensor_data&& other) = default;
    sensor_data &operator=(sensor_data&& other) = default;

    sensor_data(const sensor_data &other) = delete;
    sensor_data &operator=(const sensor_data& other) = delete;

    sensor_data(rc_Timestamp timestamp_us, rc_SensorType sensor_type, rc_Sensor sensor_id,
                rc_Timestamp shutter_time_us, int width, int height, int stride, rc_ImageFormat format, const void * image_ptr,
                void (*release)(void * handle), void * handle) :
        timestamp(sensor_clock::micros_to_tp(timestamp_us + shutter_time_us / 2))
    {
        assert(sensor_type == rc_SENSOR_TYPE_IMAGE || sensor_type == rc_SENSOR_TYPE_DEPTH);
        id = sensor_id;
        type = sensor_type;
        time_us = timestamp_us;
        image.shutter_time_us = shutter_time_us;
        image.width = width;
        image.height = height;
        image.stride = stride;
        image.format = format;
        image.image = image_ptr;
        image.release = release;
        image.handle = handle;
        image_handle = std::unique_ptr<void, void(*)(void *)>(handle, release);
    }

    sensor_data(rc_Timestamp timestamp_us, rc_SensorType sensor_type, rc_Sensor sensor_id,
                rc_Vector data) :
        timestamp(sensor_clock::micros_to_tp(timestamp_us))
    {
        assert(sensor_type == rc_SENSOR_TYPE_ACCELEROMETER || sensor_type == rc_SENSOR_TYPE_GYROSCOPE);
        id = sensor_id;
        type = sensor_type;
        time_us = timestamp_us;
        if(sensor_type == rc_SENSOR_TYPE_ACCELEROMETER)
            acceleration_m__s2 = data;
        else
            angular_velocity_rad__s = data;
    }

    std::unique_ptr<sensor_data> make_copy() const
    {
        switch(type) {
        case rc_SENSOR_TYPE_IMAGE:
        case rc_SENSOR_TYPE_DEPTH:
            {
            assert(image.height && image.stride);
            void * res_image = malloc(image.stride*image.height);
            memcpy(res_image, image.image, image.stride*image.height);
            return std::make_unique<sensor_data>(time_us, type, id,
                      image.shutter_time_us, image.width, image.height, image.stride, image.format,
                      res_image, [](void * handle) {
                        free(handle);
                      }, res_image);
            }
            break;

        case rc_SENSOR_TYPE_ACCELEROMETER:
            return std::make_unique<sensor_data>(time_us, type, id, acceleration_m__s2);
            break;

        case rc_SENSOR_TYPE_GYROSCOPE:
            return std::make_unique<sensor_data>(time_us, type, id, angular_velocity_rad__s);
            break;
        }
    }
};

#endif
