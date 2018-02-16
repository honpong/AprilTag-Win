#include <stdlib.h>
#include <iostream>
#include <mqueue.h>
#include <sys/ioctl.h>
#include <bsp.h>
#include <mv_types.h>
#include <rtems.h>
#include <inttypes.h>

#include "Trace.h"
#include "tm2_playback_stream.h"
#include "usbpump_application_rtems_api.h"
#include "usbpumpdebug.h"
#include "tm2_outcall.h"
#include "packet_io.h"
#include "usbpump_vsc2app.h"
#include "resource.h"
#include "usb_definitions.h"

#ifndef DISABLE_LEON_DCACHE
#define USBPUMP_MDK_CACHE_ENABLE 1
#else
#define USBPUMP_MDK_CACHE_ENABLE 0
#endif

// USB VSC Handler
extern USBPUMP_VSC2APP_CONTEXT *               pSelf;

static USBPUMP_APPLICATION_RTEMS_CONFIGURATION sg_DataPump_AppConfig = 
    USBPUMP_APPLICATION_RTEMS_CONFIGURATION_INIT_V1(
    /* nEventQueue */ 64,
    /* pMemoryPool */ NULL,
    /* nMemoryPool */ 0,
    /* DataPumpTaskPriority */ 100,
    /* DebugTaskPriority */ 200,
    /* UsbInterruptPriority */ 10,
    /* pDeviceSerialNumber */ NULL,
    /* pUseBusPowerFn */ NULL,
    /* fCacheEnabled */ USBPUMP_MDK_CACHE_ENABLE,
    /* DebugMask */ UDMASK_ANY | UDMASK_ERRORS);

static bool blocking_write(packet_t *pkt) {
    return usb_blocking_write(ENDPOINT_DEV_INT_OUT, (uint8_t *)pkt, pkt->header.bytes);
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
    static size_t transferred_bytes = 0;
    tm2_pb_stream::stream_buffer * stream = (tm2_pb_stream::stream_buffer *)handle;
    if (stream->content.empty() && transferred_bytes < stream->total_bytes) transferred_bytes += get_next_packet(*stream);

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

std::unique_ptr<tm2_pb_stream> tm2_pb_stream::singleton;

static void status_callback(void * handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence) {
    printf("Status change: state %d error %d confidence %d\n", state, error, confidence);
}

static void transfer_track_output(const replay_output *output, const rc_Data *data) {
    START_EVENT(EV_REPLAY_TRANSFER, 0);
    size_t data_size = 0;
    packet_t *output_packet = nullptr;
    output->get((void **)&output_packet, &data_size);
    static rc_packet_t pose_pkt = packet_command_alloc(packet_rc_pose);
    output_packet->header = pose_pkt->header;
    output_packet->header.bytes = (uint32_t)(data_size);
    if (!blocking_write(output_packet))
        printf("Error writing 6dof\n");
    END_EVENT(EV_REPLAY_TRANSFER, 0);
}

tm2_pb_stream::tm2_pb_stream(): str_buf(new stream_buffer()), track_output(new replay_output()){
    track_output->on_track_output = transfer_track_output;
    device_stream::pose_handle = track_output.get();
    device_stream::pose_callback = pose_data_callback;
    device_stream::status_callback = status_callback;
    device_stream::save_callback = usb_save_callback;
    device_stream::map_load_callback = usb_load_callback;
    device_stream::map_load_handle = str_buf.get();
}

bool tm2_pb_stream::init_device() {
    void * r = UsbPump_Rtems_DataPump_Startup(&sg_DataPump_AppConfig);
    if (!r) {
        printf("ERR: [usbInit] UPF_DataPump_Startup() failed: %p\n", r);
        return false;
    }
    usb_init();
    return true;
}

bool tm2_pb_stream::read_header(packet_header_t *header, bool control_type) {
    packet = { nullptr, free };
    if (!packet_io_read(packet)) {
        printf("Error: failed to read packet.\n");
        return false;
    }
    switch (get_packet_type(packet)) {
    case packet_enable_features_output: { track_output->set_output_type(replay_output::output_mode::POSE_FEATURE); break; }
    case packet_load_map: {
        ((stream_buffer *)device_stream::map_load_handle)->total_bytes = ((uint64_t *)packet->data)[0];
        break;
    }
    };
    if (header) *header = packet->header;
    return true;
}

void tm2_pb_stream::put_device_packet(const rc_packet_t &device_packet) {
    if (!device_packet) return;
    if (!packet_io_write(device_packet))
        printf("Error: failed to post device packet.\n");
    if (get_packet_type(device_packet) == packet_command_end) {
        rc_packet_t pose_pkt = packet_control_alloc(packet_command_end, NULL,
            track_output->get_buffer_size() - sizeof(packet_header_t));
        if (!blocking_write(pose_pkt.get()))
            printf("Error writing 6dof\n");
    }
}

