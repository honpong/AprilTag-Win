//
//  capture.cpp
//
//  Created by Brian Fulkerson on 4/15/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "capture.h"

#include <assert.h>
#include <string.h>
#include "device_parameters.h"
#include "../cor/packet.h"
#include "../cor/sensor_data.h"

packet_t *packet_alloc(enum packet_type type, uint32_t bytes_, uint64_t time)
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
    ptr->header.user = 0;
    memset(ptr->data + bytes_, 0, bytes - bytes_); // zero the padding
    return ptr;
}

void capture::write_packet(packet_t * p)
{
    file.write((const char *)p, p->header.bytes);
    bytes_written += p->header.bytes;
    packets_written++;
}

void capture::write_accelerometer_data(const float data[3], uint64_t timestamp)
{
    auto bytes = 3*sizeof(float);
    packet_t *buf = packet_alloc(packet_accelerometer, bytes, timestamp);
    memcpy(buf->data, data, bytes);
    write_packet(buf);
    free(buf);
}

void capture::write_gyroscope_data(const float data[3], uint64_t timestamp)
{
    auto bytes = 3*sizeof(float);
    packet_t *buf = packet_alloc(packet_gyroscope, bytes, timestamp);
    memcpy(buf->data, data, bytes);
    write_packet(buf);
    free(buf);
}

void capture::write_image_raw(const sensor_clock::time_point & timestamp, const sensor_clock::duration & exposure_time, const uint8_t * image, uint16_t width, uint16_t height, uint16_t stride, rc_ImageFormat format)
{
    int format_size = sizeof(uint8_t);
    if(format == rc_FORMAT_DEPTH16)
        format_size = sizeof(uint16_t);

    auto bytes = 16 + width * height * format_size;
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count();
    packet_t *buf = packet_alloc(packet_image_raw, bytes, micros);
    packet_image_raw_t *ip = (packet_image_raw_t *)buf;


    ip->width = width;
    ip->height = height;
    ip->stride = width*format_size;
    ip->exposure_time_us = std::chrono::duration_cast<std::chrono::microseconds>(exposure_time).count();
    ip->format = format;
    for(int y = 0 ; y < height; ++y)
    {
        memcpy(ip->data + y * width*format_size, image + y * stride, width*format_size);
    }

    write_packet(buf);
    free(buf);
}

void capture::write_camera(image_gray8 &&data)
{
    process(std::move([this, data=std::move(data)]() {
        write_image_raw(data.timestamp, data.exposure_time, (uint8_t *)data.image, data.width, data.height, data.stride, rc_FORMAT_GRAY8);
    }));
}

void capture::write_camera(image_depth16 &&data)
{
    process(std::move([this, data=std::move(data)]() {
        write_image_raw(data.timestamp, data.exposure_time, (uint8_t *)data.image, data.width, data.height, data.stride, rc_FORMAT_DEPTH16);
    }));
}

void capture::write_accelerometer(accelerometer_data &&data)
{
    process(std::move([this, data=std::move(data)]() {
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(data.timestamp.time_since_epoch()).count();
        write_accelerometer_data(data.accel_m__s2, micros);
    }));
}

void capture::write_gyro(gyro_data &&data)
{
    process(std::move([this, data=std::move(data)]() {
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(data.timestamp.time_since_epoch()).count();
        write_gyroscope_data(data.angvel_rad__s, micros);
    }));
}

void capture::process(std::function<void()> &&write) {
    if (!started_)
        return;
    if (threaded) {
        {
            std::lock_guard<std::mutex> queue_lock(queue_mutex);
            queue.push(std::move(write));
        }
        cv.notify_one();
    } else {
        write();
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
                    std::function<void()> write;
                    std::swap(write, queue.front());
                    queue.pop();
                    queue_lock.unlock();
                    write();
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
