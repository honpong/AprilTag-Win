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
#include <cstring>
#include <cstdlib>
#include <atomic>
#include <assert.h>
#include "rc_tracker.h"
#include "tracker.h"
#include "platform/sensor_clock.h"

class sensor_data : public rc_Data
{
private:
    std::unique_ptr<void, void(*)(void *)> image_handle{nullptr, nullptr}; // cannot assume image_handle.get() == image.image

public:
    sensor_clock::time_point timestamp;

    sensor_data() = default;

    sensor_data(sensor_data&& other) = default;
    sensor_data &operator=(sensor_data&& other) = default;

    sensor_data(const sensor_data &other) = delete;
    sensor_data &operator=(const sensor_data& other) = delete;

    friend constexpr bool operator<(const sensor_data &a, const sensor_data &b) {
        return a.timestamp == b.timestamp ? (a.type == b.type ? a.id < b.id : a.type < b.type) : a.timestamp < b.timestamp;
    }

    sensor_data(rc_Timestamp timestamp_us, rc_SensorType sensor_type, rc_Sensor sensor_id,
                rc_Timestamp shutter_time_us, int width, int height, int stride1, int stride2,
                rc_ImageFormat format, const void * image1_ptr, const void * image2_ptr,
                std::unique_ptr<void, void(*)(void *)> handle) :
        rc_Data({}),
        image_handle(std::move(handle)),
        timestamp(sensor_clock::micros_to_tp(timestamp_us + shutter_time_us / 2))
    {
        assert(sensor_type == rc_SENSOR_TYPE_STEREO);

        id = sensor_id;
        type = sensor_type;
        time_us = timestamp_us;
        path = rc_DATA_PATH_SLOW;
        stereo.shutter_time_us = shutter_time_us;
        stereo.width = width;
        stereo.height = height;
        stereo.stride1 = stride1;
        stereo.stride2 = stride2;
        stereo.format = format;
        stereo.image1 = image1_ptr;
        stereo.image2 = image2_ptr;
        stereo.handle = image_handle.get();
        stereo.release = image_handle.get_deleter();
    }

    sensor_data(rc_Timestamp timestamp_us, rc_SensorType sensor_type, rc_Sensor sensor_id,
                rc_Timestamp shutter_time_us, int width, int height, int stride, rc_ImageFormat format, const void * image_ptr,
                std::unique_ptr<void, void(*)(void *)> handle) :
        rc_Data({}),
        image_handle(std::move(handle)),
        timestamp(sensor_clock::micros_to_tp(timestamp_us + shutter_time_us / 2))
    {
        assert(sensor_type == rc_SENSOR_TYPE_IMAGE || sensor_type == rc_SENSOR_TYPE_DEPTH);
        id = sensor_id;
        type = sensor_type;
        time_us = timestamp_us;
        path = rc_DATA_PATH_SLOW;
        image.shutter_time_us = shutter_time_us;
        image.width = width;
        image.height = height;
        image.stride = stride;
        image.format = format;
        image.image = image_ptr;
        image.handle = image_handle.get();
        image.release = image_handle.get_deleter();
    }

    sensor_data(rc_Timestamp timestamp_us, rc_SensorType sensor_type, rc_Sensor sensor_id,
                rc_Vector data) :
        rc_Data({}),
        timestamp(sensor_clock::micros_to_tp(timestamp_us))
    {
        assert(sensor_type == rc_SENSOR_TYPE_ACCELEROMETER || sensor_type == rc_SENSOR_TYPE_GYROSCOPE || sensor_type == rc_SENSOR_TYPE_VELOCIMETER);
        id = sensor_id;
        type = sensor_type;
        time_us = timestamp_us;
        if(sensor_type == rc_SENSOR_TYPE_ACCELEROMETER)
            acceleration_m__s2 = data;
        else if(sensor_type == rc_SENSOR_TYPE_GYROSCOPE)
            angular_velocity_rad__s = data;
        else if(sensor_type == rc_SENSOR_TYPE_VELOCIMETER)
            translational_velocity_m__s = data;
    }

    sensor_data(rc_Timestamp timestamp_us, rc_SensorType sensor_type, rc_Sensor sensor_id, float temp_C) :
        rc_Data({}),
        timestamp(sensor_clock::micros_to_tp(timestamp_us))
    {
        assert(sensor_type == rc_SENSOR_TYPE_THERMOMETER);
        id = sensor_id;
        type = sensor_type;
        time_us = timestamp_us;
        temperature_C = temp_C;
    }

    class stack_copy {};
    sensor_data(const sensor_data &other, stack_copy) :
        rc_Data(static_cast<rc_Data>(other)),
        timestamp(other.timestamp) {
    }

    class data_copy {};
    sensor_data(const sensor_data &other, data_copy) : sensor_data(other, stack_copy()) {
        if (type == rc_SENSOR_TYPE_IMAGE || type == rc_SENSOR_TYPE_DEPTH) {
            image_handle = std::unique_ptr<void, void(*)(void *)>(
                image.handle  = malloc(image.stride*image.height),
                image.release = free
            );
            image.image = memcpy(image.handle, other.image.image, image.stride*image.height);
        }
        else if(type == rc_SENSOR_TYPE_STEREO) {
            image_handle = std::unique_ptr<void, void(*)(void *)>(
                stereo.handle  = malloc(stereo.stride1*stereo.height + stereo.stride2*stereo.height),
                stereo.release = free
            );
            stereo.image1 = memcpy(stereo.handle                               , other.stereo.image1, stereo.stride1*stereo.height);
            stereo.image2 = memcpy((uint8_t*)stereo.handle + stereo.stride1*stereo.height, other.stereo.image2, stereo.stride2*stereo.height);
        }
    }

    std::unique_ptr<sensor_data> make_copy() const
    {
        return std::make_unique<sensor_data>(*this, data_copy());
    }

    tracker::image tracker_image() const {
        assert(type == rc_SENSOR_TYPE_IMAGE);
        tracker::image timage;
        timage.image = (uint8_t *)image.image;
        timage.width_px = image.width;
        timage.height_px = image.height;
        timage.stride_px = image.stride;
        return timage;
    }

    static std::pair<sensor_data,sensor_data> split(sensor_data &&d) {
        assert(d.type == rc_SENSOR_TYPE_STEREO);
        struct counted_handle {
            std::atomic<int> count;
            std::unique_ptr<void, void(*)(void *)> handle;
        } *handle = new counted_handle{{2}, std::move(d.image_handle)};
        auto release = [](void*p) { auto h = (counted_handle *)p; if (!--h->count) delete h; };
        d.stereo.handle = nullptr;
        d.stereo.release = nullptr;
        return std::pair<sensor_data,sensor_data>(
            sensor_data{d.time_us, rc_SENSOR_TYPE_IMAGE, rc_Sensor(d.id+0), d.stereo.shutter_time_us, d.stereo.width, d.stereo.height, d.stereo.stride1, d.stereo.format, d.stereo.image1, {handle, release}},
            sensor_data{d.time_us, rc_SENSOR_TYPE_IMAGE, rc_Sensor(d.id+1), d.stereo.shutter_time_us, d.stereo.width, d.stereo.height, d.stereo.stride2, d.stereo.format, d.stereo.image2, {handle, release}}
        );
    }

    const static rc_Sensor MAX_SENSORS = 64;

    static uint64_t get_global_id_by_type_id(rc_SensorType type, rc_Sensor id) { return type * MAX_SENSORS + id; }

    static rc_SensorType get_type_by_global_id(uint64_t global_id) { return static_cast<rc_SensorType>(global_id / MAX_SENSORS); }

    static rc_Sensor get_id_by_global_id(uint64_t global_id) { return global_id % MAX_SENSORS; }

    uint64_t global_id() const { return get_global_id_by_type_id(type, id); }
};

#endif