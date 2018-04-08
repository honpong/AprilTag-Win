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
#include "sensor_data.h"
#include "packet.h"
#include <thread>
#include <atomic>
#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

class capture
{
private:
    std::ofstream file;
    uint64_t packets_written{0};
    uint64_t bytes_written{0};
    std::atomic<bool> started_ {false};
    std::mutex queue_mutex;
    std::thread thread;
    std::queue<std::unique_ptr<sensor_data> > queue;
    std::condition_variable cv;
    bool threaded = false;

    void write_packet(packet_t * p);
    void write_accelerometer_data(uint16_t sensor_id, uint64_t timestamp_us, const float data[3]);
    void write_gyroscope_data(uint16_t sensor_id, uint64_t timestamp_us, const float data[3]);
    void write_image_raw(uint16_t sensor_id, uint64_t timestamp_us, uint64_t exposure_time_us,
            const uint8_t * image, uint16_t width, uint16_t height, uint16_t stride, rc_ImageFormat format);
    void write_temperature_data(uint16_t sensor_id, uint64_t timestamp_us, float temp_C);
    void write_velocimeter_data(uint16_t sensor_id, uint64_t timestamp_us, const float data[3]);
    void write(std::unique_ptr<sensor_data> data);

public:
    bool start(const char *name, bool threaded);
    bool started() { return started_; }
    void stop();
    void push(std::unique_ptr<sensor_data> data);
    uint64_t get_bytes_written() { return bytes_written; }
    uint64_t get_packets_written() { return packets_written; }
    ~capture() { stop(); }
};

#endif /* defined(__RC3DK__capture__) */
