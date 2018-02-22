#pragma once

#include <chrono>
#include <cinttypes>
#include <thread>
#include <mutex>
#include <list>
#include <fstream>

#include "stream_object.h"

class tm2_host_stream : public host_stream {
public:
    tm2_host_stream(const char*filename);
    bool init_stream() override;
    bool start_stream() override;
    uint64_t get_bytes_dispatched() override { return bytes_dispatched; };
    uint64_t get_packets_dispatched() override { return packets_dispatched; };
    bool put_host_packet(rc_packet_t &&post_packet) override;
    void wait_device_packet(const std::vector<uint32_t> &pkt_types) override {
        if (is_usb_ok) host_stream::wait_device_packet(pkt_types);
    }
    ~tm2_host_stream();
private:
    std::ifstream sensor_file;
    std::ofstream save_file;
    size_t sensor_data_size{ 1 };
    rc_packet_t packet{ nullptr, free };
    uint64_t bytes_dispatched{ 0 };
    uint64_t packets_dispatched{ 0 };
    std::thread thread_receive_device; //to receive flow control and non-pose APIs
    std::thread thread_6dof_output; //to receive tracking results
    std::mutex put_mutex; //serialize posting of packets
    bool enable_sensor{ true }; //control delivery of sensor packets
    replay_output track_output;
    std::list<std::unique_ptr<rc_Data, void(*)(void *)>> sensor_queue;
    std::mutex image_queue_mtx;
    bool is_usb_ok{ false };
};
