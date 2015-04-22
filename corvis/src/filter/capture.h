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
#include "../cor/sensor_fusion_queue.h"
#include "../cor/packet.h"

class capture
{
private:
    std::ofstream file;
    std::atomic<uint64_t> packets_written{0};
    std::atomic<uint64_t> bytes_written{0};
    std::atomic<bool> is_running{false};
    std::unique_ptr<fusion_queue> queue;
    std::mutex write_lock;

    void write_packet(packet_t * p);
    void write_camera_data(uint8_t * image, int width, int height, int stride, uint64_t timestamp);
    void write_accelerometer_data(const float data[3], uint64_t timestamp);
    void write_gyroscope_data(const float data[3], uint64_t timestamp);
    void setup_queue();

public:
    bool start(const char *name);
    void stop();
    //TODO: not sure if this is right or not
    void receive_camera(camera_data&& x) { queue->receive_camera(std::move(x)); }
    void receive_accelerometer(accelerometer_data&& x) { queue->receive_accelerometer(std::move(x)); }
    void receive_gyro(gyro_data&& x) { queue->receive_gyro(std::move(x)); }
    uint64_t get_bytes_written() { return bytes_written; }
    uint64_t get_packets_written() { return packets_written; }
};

#endif /* defined(__RC3DK__capture__) */
