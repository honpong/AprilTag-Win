#pragma once
#include <cstdint>
#include "stream_object.h"

class tm2_pb_stream : public device_stream {
public:
    tm2_pb_stream();
    ///data to pass to each callback by rc_loadMap/rc_saveMap that includes total expected size of
    ///map content and the new map data to be processed.
    typedef struct stream_buffer {
        uint64_t total_bytes{ 0 };
        uint64_t transfer_bytes{ 0 };
        std::deque<std::string> content;  ///map data to be processed
    } stream_buffer;

    bool init_device() override;
    bool read_header(packet_header_t *header, bool control_type = false) override;
    rc_packet_t get_host_packet() override { return std::move(packet); }
    void put_device_packet(const rc_packet_t &post_packet) override;
private:
    rc_packet_t packet{ nullptr, free };
    std::unique_ptr<stream_buffer> str_buf;
    replay_output track_output[2];
};