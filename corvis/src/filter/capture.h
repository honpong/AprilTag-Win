//
//  capture.h
//
//  Created by Brian Fulkerson on 4/15/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__capture__
#define __RC3DK__capture__

#include <iostream>
#include <fstream>
#include "../cor/sensor_data.h"
#include "../cor/packet.h"
#include <thread>
#include <atomic>
#include <mutex>

class capture
{
private:
    std::ofstream file;
    std::atomic<uint64_t> packets_written{0};
    std::atomic<uint64_t> bytes_written{0};
    std::atomic<bool> is_running{false};
    std::mutex write_lock;

    void write_packet(packet_t * p);
    void write_accelerometer_data(const float data[3], uint64_t timestamp);
    void write_gyroscope_data(const float data[3], uint64_t timestamp);
    void write_image_raw(const sensor_clock::time_point & timestamp, const sensor_clock::duration & exposure_time,
            const uint8_t * image, uint16_t width, uint16_t height, uint16_t stride, rc_ImageFormat format);

public:
    bool start(const char *name);
    void stop();
    void write_camera(const image_gray8& x);
    void write_camera(const image_depth16& x);
    void write_accelerometer(const accelerometer_data& x);
    void write_gyro(const gyro_data& x);
    uint64_t get_bytes_written() { return bytes_written; }
    uint64_t get_packets_written() { return packets_written; }
};

#endif /* defined(__RC3DK__capture__) */
