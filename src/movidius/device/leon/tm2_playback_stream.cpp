#include <stdlib.h>
#include <iostream>
#include "tm2_outcall.h"
#include "packet_io.h"
#include "usb_definitions.h"
#include "mv_trace.h"
#include "tm2_playback_stream.h"

static bool blocking_write_same_size_pkt(packet_t *pkt) {
    return usb_blocking_write(ENDPOINT_DEV_INT_OUT, (uint8_t *)pkt, pkt->header.bytes);
}

static bool blocking_write_ack_pkt(uint64_t ack_us) {
    static rc_packet_t ack_pkt = packet_command_alloc(packet_sensor_ack);
    ack_pkt->header.time = ack_us;
    return usb_blocking_write(ENDPOINT_DEV_OUT, (uint8_t *)ack_pkt.get(), ack_pkt->header.bytes);
}

static size_t get_next_packet(tm2_pb_stream::stream_buffer &s)
{
    rc_packet_t usb_packet(nullptr, free);
    size_t     packet_size = 0;
    if (packet_io_read(usb_packet)) {
        packet_size = usb_packet->header.bytes - sizeof(packet_header_t);
        s.content.push_back(std::string((const char *)usb_packet->data, packet_size));
    }
    else
        printf("Error: failed reading packet \n");
    return packet_size;
}

/// callback function for e.g. rc_loadMap to stream map data over usb to the
/// on-the-fly map reconstruction. Stream data is put in a buffer for processing
/// to accommodate flexible sizing of streaming buffer that could be different from
/// the size of internal loading buffer.
static size_t usb_load_callback(void * handle, void * buffer, size_t length)
{
    tm2_pb_stream::stream_buffer * stream = (tm2_pb_stream::stream_buffer *)handle;
    if (stream->content.empty() && stream->transfer_bytes < stream->total_bytes) stream->transfer_bytes += get_next_packet(*stream);

    size_t buffer_copy_size = 0;
    if (!stream->content.empty()) {
        buffer_copy_size = std::min(stream->content.front().size(), length);
        memcpy(buffer, stream->content.front().c_str(), buffer_copy_size);
        if (buffer_copy_size < stream->content.front().size())
            stream->content[0] = stream->content.front().substr(buffer_copy_size);
        else
            stream->content.pop_front();
    }
    return buffer_copy_size;
}

static void usb_save_callback(void * handle, const void * buffer, size_t length)
{
    static packet_header_t   save_packet_header = { sizeof(packet_header_t), packet_control, packet_save_data, 0 };
    save_packet_header.bytes = length + sizeof(packet_header_t);
    if (!packet_io_write_parts(&save_packet_header, (const uint8_t *)buffer))
        printf("Error: failed to send packet for saving\n");
}

static void status_callback(void * handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence) {
    printf("Status change: state %d error %d confidence %d\n", state, error, confidence);
}

/// output only includes the correct pose type
static void transfer_track_output(const replay_output *output, const rc_Data *data) {
    START_EVENT(EV_REPLAY_TRANSFER, 0);
    size_t data_size = 0;
    packet_t *output_packet = nullptr;
    output->get((void **)&output_packet, &data_size);
    static rc_packet_t pose_pkt = packet_command_alloc(packet_rc_pose);
    output_packet->header = pose_pkt->header;
    output_packet->header.bytes = (uint32_t)(data_size);
    if (!blocking_write_same_size_pkt(output_packet))
        printf("Error writing 6dof\n");
    END_EVENT(EV_REPLAY_TRANSFER, 0);
}

static void process_relocalization(void *, rc_Tracker *, const rc_Relocalization *reloc) {
    rc_packet_t reloc_pkt = packet_control_alloc(packet_rc_relocalization, nullptr, sizeof(packet_rc_relocalization_t) - sizeof(packet_header_t));
    packet_rc_relocalization_t* reloc_pkt_p = (packet_rc_relocalization_t*)reloc_pkt.get();
    reloc_pkt_p->timestamp = reloc->time_us;
    reloc_pkt_p->sessionid = reloc->session;
    packet_io_write(reloc_pkt);
}

tm2_pb_stream::tm2_pb_stream(): str_buf(new stream_buffer()) {
    track_output[rc_DATA_PATH_FAST].on_track_output = transfer_track_output;
    track_output[rc_DATA_PATH_SLOW].on_track_output = transfer_track_output;
    device_stream::pose_handle = &track_output;
    device_stream::pose_callback = pose_data_callback;
    device_stream::status_callback = status_callback;
    device_stream::save_callback = usb_save_callback;
    device_stream::map_load_callback = usb_load_callback;
    device_stream::map_load_handle = str_buf.get();
    device_stream::relocalization_callback = process_relocalization;
}

bool tm2_pb_stream::init_device() {
    return true;
}

bool tm2_pb_stream::read_header(packet_header_t *header, bool control_type) {
    packet = { nullptr, free };
    if (!packet_io_read(packet)) return false;
    switch (get_packet_type(packet)) {
    case packet_enable_output_mode: {
        auto mode = (replay_output::output_mode)((const uint8_t *)packet->data)[0];
        track_output[rc_DATA_PATH_SLOW].set_output_type(mode);
        track_output[rc_DATA_PATH_FAST].set_output_type(mode); //need to be of same size
        break;
    }
    case packet_load_map: {
        ((stream_buffer *)device_stream::map_load_handle)->total_bytes = ((uint64_t *)packet->data)[0];
        ((stream_buffer *)device_stream::map_load_handle)->transfer_bytes = 0;
        break;
    }
    case packet_command_start: { is_started = true; break; }
    case packet_enable_usb_sync: { usb_sync = true; break; }
    };
    if (header) *header = packet->header;
    return true;
}

void tm2_pb_stream::put_device_packet(const rc_packet_t &device_packet) {
    if (!device_packet || !is_started) return;
    if (!packet_io_write(device_packet))
        printf("Error: failed to post device packet.\n");
    if (get_packet_type(device_packet) == packet_command_end) {
        track_output[rc_DATA_PATH_SLOW].rc_resetFeatures();
        track_output[rc_DATA_PATH_FAST].rc_resetFeatures();
        rc_packet_t pose_pkt = packet_control_alloc(packet_command_end, NULL,
            track_output[rc_DATA_PATH_SLOW].get_buffer_size() - sizeof(packet_header_t));
        if (!blocking_write_same_size_pkt(pose_pkt.get()))
            printf("Error writing 6dof\n");
    }
}

void tm2_pb_stream::device_ack(uint64_t ack_us) {
    if (usb_sync) blocking_write_ack_pkt(ack_us);
}
