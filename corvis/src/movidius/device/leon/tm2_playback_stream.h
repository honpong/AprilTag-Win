#pragma once
#include <cstdint>
#include "stream_object.h"

class tm2_pb_stream : public device_stream {
public:
    ///data to pass to each callback by rc_loadMap/rc_saveMap that includes total expected size of
    ///map content and the new map data to be processed.
    typedef struct stream_buffer {
        uint64_t total_bytes{ 0 };
        std::deque<std::string> content;  ///map data to be processed
    } stream_buffer;

    static device_stream *get_instance() {
        if (!singleton) singleton = std::unique_ptr<tm2_pb_stream>(new tm2_pb_stream());
        return singleton.get();
    }
    bool init_device() override;
    bool read_header(packet_header_t *header, bool control_type = false) override;
    rc_packet_t get_host_packet() override { return std::move(packet); }
    void put_device_packet(const rc_packet_t &post_packet) override;
private:
    rc_packet_t packet{ nullptr, free };
    std::unique_ptr<stream_buffer> str_buf{ nullptr };
    std::unique_ptr<replay_output> track_output{ nullptr };
    static std::unique_ptr<tm2_pb_stream> singleton;
    tm2_pb_stream();
};