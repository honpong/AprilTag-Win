#pragma once
#include <fstream>
#include "stream_object.h"
#include "platform/sensor_clock.h"

class replay_device;

/// streaming data from a file
class file_stream : public device_stream, public host_stream {
private:
    std::ifstream sensor_file;
    std::ofstream save_file;
    bool enable_sensor = false; //initially disable sensor packets
    float sensor_data_size{ 1 };
    rc_packet_t packet{ nullptr, free };
    uint64_t bytes_dispatched{ 0 };
    uint64_t packets_dispatched{ 0 };
    sensor_clock::time_point last_progress;
    rc_packet_t map_data{ nullptr, free };
    std::unique_ptr<replay_output::bstream_buffer> stream_handle;
    replay_output track_output[2];
    std::deque<rc_packet_t> host_queue;
    std::mutex host_queue_mtx;
    std::condition_variable control_packet_cond;
    replay_device *rp_device{ nullptr };
public:
    file_stream(const char *name);
    bool init_stream() override;
    bool init_device() override { return true; };
    bool start_stream() override;
    bool read_header(packet_header_t *header, bool control_only = false) override;
    rc_packet_t get_host_packet() override { return std::move(packet);}
    void put_device_packet(const rc_packet_t &post_packet) override;
    uint64_t get_bytes_dispatched() override { return bytes_dispatched; };
    uint64_t get_packets_dispatched()override { return packets_dispatched; };
    bool put_host_packet(rc_packet_t &&post_packet) override;
    ~file_stream();
};
