//
//  capture.cpp
//
//  Created by Brian Fulkerson on 4/15/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "capture.h"

#include <string.h>
#include "device_parameters.h"
#include "../cor/packet.h"
#include "../cor/sensor_data.h"

packet_t *packet_alloc(enum packet_type type, uint32_t bytes, uint64_t time)
{
    //add 7 and mask to pad to 8-byte boundary
    bytes = ((bytes + 7) & 0xfffffff8u);
    //header
    bytes += 16;

    packet_t * ptr = (packet_t *)malloc(bytes);
    ptr->header.type = type;
    ptr->header.bytes = bytes;
    ptr->header.time = time;
    ptr->header.user = 0;
    return ptr;
}

void capture::write_packet(packet_t * p)
{
    write_lock.lock();
    file.write((const char *)p, p->header.bytes);
    write_lock.unlock();
    bytes_written += p->header.bytes;
    packets_written++;
}

void capture::write_image_gray8(uint8_t * image, int width, int height, int stride, uint64_t timestamp)
{
    // 16 bytes for pgm header
    packet_t *buf = packet_alloc(packet_camera, width*height+16, timestamp);

    // TODO: For 1080 this could be a problem?
    snprintf((char *)buf->data, 16, "P5 %4d %4d %d\n", width, height, 255);
    unsigned char *outbase = buf->data + 16;
    memcpy(outbase, image, width*height);
    write_packet(buf);
    free(buf);
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

void capture::receive_image_gray8(const image_gray8 &data)
{
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(data.timestamp.time_since_epoch()).count();
    write_image_gray8(data.image, data.width, data.height, data.stride, micros);
};

void capture::receive_camera(const camera_data &data)
{
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(data.timestamp.time_since_epoch()).count();
    write_image_gray8(data.image, data.width, data.height, data.stride, micros);
};

void capture::receive_accelerometer(const accelerometer_data &data)
{
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(data.timestamp.time_since_epoch()).count();
    write_accelerometer_data(data.accel_m__s2, micros);
}

void capture::receive_gyro(const gyro_data &data)
{
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(data.timestamp.time_since_epoch()).count();
    write_gyroscope_data(data.angvel_rad__s, micros);
}

bool capture::start(const char *name)
{
    file.open(name, std::ios::binary);
    if(file.bad())
    {
        std::cerr << "Couldn't open file " << name << " for writing.\n";
        return false;
    }
    return true;
}

void capture::stop()
{
    file.flush();
    file.close();
}