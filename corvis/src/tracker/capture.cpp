//
//  capture.cpp
//
//  Created by Brian Fulkerson on 4/15/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "capture.h"

#include <assert.h>
#include <string.h>
#include "packet.h"
#include "sensor_data.h"

packet_t *packet_alloc(enum packet_type type, uint32_t bytes_, uint16_t sensor_id, uint64_t time)
{
    //add 7 and mask to pad to 8-byte boundary
    uint32_t bytes = ((bytes_ + 7) & 0xfffffff8u);

    packet_t * ptr = (packet_t *)malloc(sizeof(packet_header_t) + bytes);
    if(!ptr)
    {
        fprintf(stderr, "Capture failed: malloc returned null.\n");
        assert(0);
    }
    ptr->header.type = type;
    ptr->header.bytes = sizeof(packet_header_t) + bytes;
    ptr->header.time = time;
    ptr->header.sensor_id = sensor_id;
    memset(ptr->data + bytes_, 0, bytes - bytes_); // zero the padding
    return ptr;
}

void capture::write_packet(packet_t * p)
{
    file.write((const char *)p, p->header.bytes);
    bytes_written += p->header.bytes;
    packets_written++;
}

void capture::write_temperature_data(uint16_t sensor_id, uint64_t timestamp_us, float temp_C)
{
    uint32_t bytes = sizeof(float);
    packet_t *buf = packet_alloc(packet_thermometer, bytes, sensor_id, timestamp_us);
    memcpy(buf->data, &temp_C, bytes);
    write_packet(buf);
    free(buf);
}

void capture::write_accelerometer_data(uint16_t sensor_id, uint64_t timestamp_us, const float data[3])
{
    uint32_t bytes = 3*sizeof(float);
    packet_t *buf = packet_alloc(packet_accelerometer, bytes, sensor_id, timestamp_us);
    memcpy(buf->data, data, bytes);
    write_packet(buf);
    free(buf);
}

void capture::write_gyroscope_data(uint16_t sensor_id, uint64_t timestamp_us, const float data[3])
{
    uint32_t bytes = 3*sizeof(float);
    packet_t *buf = packet_alloc(packet_gyroscope, bytes, sensor_id, timestamp_us);
    memcpy(buf->data, data, bytes);
    write_packet(buf);
    free(buf);
}

void capture::write_image_raw(uint16_t sensor_id, uint64_t timestamp_us, uint64_t exposure_time_us, const uint8_t * image, uint16_t width, uint16_t height, uint16_t stride, rc_ImageFormat format)
{
    int format_size = sizeof(uint8_t);
    if(format == rc_FORMAT_DEPTH16)
        format_size = sizeof(uint16_t);

    auto bytes = 16 + width * height * format_size;
    packet_t *buf = packet_alloc(packet_image_raw, bytes, sensor_id, timestamp_us);
    packet_image_raw_t *ip = (packet_image_raw_t *)buf;


    ip->width = width;
    ip->height = height;
    ip->stride = width*format_size;
    ip->exposure_time_us = exposure_time_us;
    ip->format = format;
    for(int y = 0 ; y < height; ++y)
    {
        memcpy(ip->data + y * width*format_size, image + y * stride, width*format_size);
    }

    write_packet(buf);
    free(buf);
}

void capture::write(std::unique_ptr<sensor_data> data)
{
    switch(data->type) {
        case rc_SENSOR_TYPE_IMAGE:
            write_image_raw(data->id, data->time_us, data->image.shutter_time_us, (uint8_t *)data->image.image, data->image.width, data->image.height, data->image.stride, rc_FORMAT_GRAY8);
            break;

        case rc_SENSOR_TYPE_DEPTH:
            write_image_raw(data->id, data->time_us, data->depth.shutter_time_us, (uint8_t *)data->depth.image, data->depth.width, data->depth.height, data->depth.stride, rc_FORMAT_DEPTH16);
            break;

        case rc_SENSOR_TYPE_ACCELEROMETER:
            write_accelerometer_data(data->id, data->time_us, data->acceleration_m__s2.v);
            break;

        case rc_SENSOR_TYPE_GYROSCOPE:
            write_gyroscope_data(data->id, data->time_us, data->angular_velocity_rad__s.v);
            break;

        case rc_SENSOR_TYPE_STEREO:
            //TODO support stereo capture
            fprintf(stderr, "Stereo capture not yet supported\n");

        case rc_SENSOR_TYPE_THERMOMETER:
            write_temperature_data(data->id, data->time_us, data->temperature_C);
            break;
    }
}

void capture::push(std::unique_ptr<sensor_data> data)
{
    if (!started_)
        return;
    if (threaded) {
        {
            std::lock_guard<std::mutex> queue_lock(queue_mutex);
            queue.push(std::move(data));
        }
        cv.notify_one();
    } else {
        write(std::move(data));
    }
}

bool capture::start(const char *name, bool threaded)
{
    if (started_)
        return false;
    file.open(name, std::ios::binary);
    if(!file.is_open())
        return false;
    started_ = true;
    if ((this->threaded = threaded))
        thread = std::thread([this]() {
            std::unique_lock<std::mutex> queue_lock(queue_mutex);
            while (started_ || !queue.empty()) {
                cv.wait(queue_lock);
                while (!queue.empty()) {
                    std::unique_ptr<sensor_data> data;
                    std::swap(data, queue.front());
                    queue.pop();
                    queue_lock.unlock();
                    write(std::move(data));
                    queue_lock.lock();
                }
            }
        });
    return true;
}

void capture::stop()
{
    if (!started_)
        return;
    started_ = false;
    if (threaded) {
        cv.notify_one();
        thread.join();
    }
    file.close();
}
