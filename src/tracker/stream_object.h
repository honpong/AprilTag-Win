#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <iomanip>

#include "bstream.h"
#include "packet.h"

struct packet_header_t;
class replay_output;

typedef std::unique_ptr<packet_t, decltype(&free)> rc_packet_t;

/// packet type used for replay, must not exceed max(uint8_t).
typedef enum replay_packet_type {
    packet_command_init = 180,
    packet_command_reset,
    packet_command_end,
    packet_command_toggle_pause,
    packet_command_next_pause,
    packet_command_step,
    packet_ack, //acknowledgment
    packet_sensor_ack, //acknowledgment of sensor packet
    packet_transfer_end,
    packet_enable_realtime,
    packet_enable_qvga,
    packet_enable_qres, //w. downscaling ratio by 0
    packet_enable_async,
    packet_enable_no_depth,
    packet_enable_fast_path,
    packet_enable_dynamic_calibration,
    packet_enable_zero_biases,
    packet_enable_mesg_level,
    packet_enable_mapping,
    packet_enable_relocalization,
    packet_enable_output_mode,
    packet_enable_odometry,
    packet_enable_usb_sync, //enable synchronous processing per packet over USB
    packet_timing_stat,
    packet_storage_stat,
    packet_load_calibration,
    packet_save_calibration,
    packet_load_map,
    packet_save_map,
    packet_save_data, //packet used by device to send for saving
    packet_save_end, //packet used by device to send for saving
    packet_set_stage,
    packet_camera_extrinsics,
    packet_delay_start,
    packet_set_queue_strategy,
    packet_invalid_usb, //last entry
} replay_packet_type;

class host_stream {
public:
    /// make stream object ready
    virtual bool init_stream() = 0;
    ///start streaming out sensor data and streaming in tracking results.
    virtual bool start_stream() = 0;
    /// post a packet to the stream object that will be read back later.
    /// if the packet is mixed with others from a different stream, the ordering w.r.t other packets is not defined.
    virtual bool put_host_packet(rc_packet_t &&post_packet) = 0;
    /// get number of bytes of sensor data dispatched in the stream.
    virtual uint64_t get_bytes_dispatched() = 0;
    /// get number of sensor packets dispatched in the stream.
    virtual uint64_t get_packets_dispatched() = 0;
    /// callback function on host side upon receipt of replay_output.
    std::function<void(const replay_output *, const rc_Data *)> host_data_callback{ nullptr };
    /// callback function on streaming progress, meaningful only for streaming from a recording file.
    std::function<void(float)> progress_callback{ nullptr };
    /// wait until a packet of ANY give type by the parameter is generated by the device program.
    virtual void wait_device_packet(const std::vector<uint32_t> &pkt_types) {
        std::unique_lock<std::mutex> lk(wait_device_mtx);
        device_response.wait(lk, [&] {
            if (arrived_type == packet_invalid_usb) return true; //release wait
            bool arrived = false;
            for (auto type : pkt_types) {
                if (arrived_type == type) {
                    arrived = true;
                    arrived_type = packet_none;
                    break;
                }
            }
            return arrived;
        });
    }
    /// tracking statistics.
    std::string tracking_stat;
    rc_StorageStats storage_stat;
    rc_Extrinsics camera_extrinsics[2] = { rc_Extrinsics{} };
    virtual ~host_stream() {};
protected:
    std::mutex wait_device_mtx;
    std::condition_variable device_response;
    uint32_t arrived_type{ 0 }; ///type of control packet sent by device program.
    static constexpr size_t max_packet_size{ 10 * 1000000 }; //maximum size of a packet
    bool stream_sts{ false };
    static constexpr int file_buffer_bytes = 128 * 1024;
    std::unique_ptr<char[]> buffer{ new char[file_buffer_bytes] };
};


class device_stream {
public:
    std::string name{};
    /// make stream object ready for streaming.
    virtual bool init_device() = 0;
    /// allocates a new packet and assigns header of the next packet in the stream.
    /// waits until there is a packet.
    virtual bool read_header(packet_header_t *header, bool control_type = false) = 0;
    /// acquire and own pointer to the current packet. This function must be called after read_header.
    /// user is required to FREE the pointer after usage.
    virtual  rc_packet_t get_host_packet() = 0;
    /// post a packet to the stream object that will be queued and read back later.
    /// after packet is read back and processed, its allocation will be freed by user. 
    virtual void put_device_packet(const rc_packet_t &post_packet) = 0;
    /// acknowledge receipt of packet
    virtual void device_ack(uint64_t ack_us) {};
    rc_LoadCallback map_load_callback{ nullptr };
    rc_SaveCallback save_callback{ nullptr };
    rc_DataCallback pose_callback{ nullptr };
    rc_MessageCallback message_callback{ nullptr };
    void * message_handle { nullptr };
    rc_StatusCallback status_callback{ nullptr };
    void *map_load_handle{ nullptr }, *save_handle{ nullptr }, *pose_handle{ nullptr };
    virtual ~device_stream() {};
};

static inline void pose_data_callback(void * handle, rc_Tracker * tracker, const rc_Data * data) {
    replay_output *output = &((replay_output *)handle)[data->path]; //get corresponding pose object to path or slow path.
    output->confidence = (int8_t)rc_getConfidence(tracker);
    output->path_length = rc_getPathLength(tracker);
    output->sensor_time_us = data->time_us;
    output->data_path = data->path;
    output->sensor_type = data->type;
    output->sensor_id = data->id;
    // update for both fast and slow path to match internal changes in tracker
    output->rc_setPose(rc_DATA_PATH_FAST, rc_getPose(tracker, nullptr, nullptr, rc_DATA_PATH_FAST));
    output->rc_setPose(rc_DATA_PATH_SLOW, rc_getPose(tracker, nullptr, nullptr, rc_DATA_PATH_SLOW));
    auto output_type = output->get_output_type();
    if (data->type == rc_SENSOR_TYPE_IMAGE || data->type == rc_SENSOR_TYPE_STEREO) {
        if (output_type == replay_output::output_mode::POSE_FEATURE ||
            output_type == replay_output::output_mode::POSE_FEATURE_MAP) {
            rc_Feature *cur_feat = nullptr;
            uint32_t num_features = rc_getFeatures(tracker, data->id, &cur_feat);
            output->rc_setFeatures(cur_feat, num_features);
        }
        if (output_type == replay_output::output_mode::POSE_MAP ||
            output_type == replay_output::output_mode::POSE_FEATURE_MAP) {
            rc_Pose *cur_reloc_poses = nullptr;
            uint32_t num_reloc_poses = rc_getRelocalizationPoses(tracker, &cur_reloc_poses);
            output->rc_setRelocPoses(cur_reloc_poses, num_reloc_poses);
        }
    }
    output->tracker = tracker;
    if (output->on_track_output) output->on_track_output(output, data);
}

static inline rc_packet_t packet_control_alloc(uint8_t control_type, const char *load, size_t load_size) {
    packet_control_t *new_packet = (packet_control_t *)malloc(sizeof(packet_header_t) + load_size);
    new_packet->header.type = packet_control;
    new_packet->header.control_type = control_type;
    new_packet->header.time = 0; //not used yet
    new_packet->header.bytes = (uint32_t)(sizeof(packet_header_t) + load_size);
    if (load) memcpy(new_packet->data, load, load_size);
    return { (packet_t *)new_packet, free };
}

static inline rc_packet_t packet_command_alloc(uint8_t control_type) {
    return packet_control_alloc(control_type, NULL, 0);
}
/// provides a type whose storage size is an upper bound in multiple of 4 bytes
template <typename T, size_t S = ((sizeof(T) + 3) / 4 * 4)>
union packet_aligned_item {
    T value;
    typename std::aligned_storage<S, alignof(T)>::type data;
};

/// creates a packet for a single data type.
template<typename T>
static inline rc_packet_t packet_single_control_alloc(uint8_t control_type, const T &value) {
    union packet_aligned_item<T> transfer = {value};
    return packet_control_alloc(control_type, (char *)&transfer, sizeof(transfer));
}

/// helper class to return data of a given type
struct packet_item_type {
    packet_item_type(const packet_t *packet_) : packet(packet_) {};
    const packet_t *packet;
    template <class T>
    operator T() {
        union packet_aligned_item<T> transfer;
        memcpy(&transfer, packet->data, sizeof(transfer));
        return transfer.value;
    }
};

/// get a single data item in the control packet whose type depends on the return type
static inline packet_item_type get_packet_item(const packet_control_t *packet) { return packet_item_type((packet_t *)packet); }
/// get a single data item in the control packet whose type depends on the return type
static inline packet_item_type get_packet_item(const rc_packet_t &packet) { return packet_item_type(packet.get()); }

static inline void set_control_packet(packet_control_t *packet, uint8_t type, const char *load,
    size_t load_size) {
    packet->header.type = packet_control;
    packet->header.control_type = type;
    packet->header.time = 0;
    packet->header.bytes = (uint32_t)(sizeof(packet_header_t) + load_size);
    if (load && load_size > 0)
        memcpy(packet->data, load, load_size);
}

static inline uint8_t get_packet_type(const rc_packet_t &packet) {
    if (packet->header.type == packet_control)
        return (uint8_t)((packet_control_t *)packet.get())->header.control_type;
    else return (uint8_t)packet->header.type;
}

static void delete_rc_data(void *data) {
    if (!data) return;
    rc_Data *rc_data = (rc_Data *)data;
    if (rc_data->type == rc_SensorType::rc_SENSOR_TYPE_IMAGE) delete[](char *)rc_data->image.image;
    delete rc_data;
}

/// for packet of type packet_stereo_raw, two rc_Data items of type rc_SENSOR_TYPE_IMAGE  is created.
/// cam_id is used to select which corresponding image is created.
static inline std::unique_ptr<rc_Data, void(*)(void *)> create_rc_Data(const rc_packet_t &packet, int cam_id = 0) {
    rc_Data *new_data = nullptr;
    switch (get_packet_type(packet)) {
    case packet_image_raw: {
        auto *ip = (packet_image_raw_t *)packet.get();
        char *content = new char[ip->stride * ip->height];
        memcpy(content, ip->data, ip->stride * ip->height);
        new_data = new rc_Data{ packet->header.sensor_id, rc_SensorType::rc_SENSOR_TYPE_IMAGE,
            (rc_Timestamp)ip->header.time, rc_DataPath::rc_DATA_PATH_SLOW, {} };
        new_data->image = rc_ImageData{ (rc_Timestamp)ip->exposure_time_us, ip->width, ip->height, ip->stride,
            (rc_ImageFormat)ip->format, content, NULL, NULL };
        break;
    }
    case packet_stereo_raw: { //generate one rc_SENSOR_TYPE_IMAGE at a time
        auto *ip = (packet_stereo_raw_t *)packet.get();
        auto img_stride = cam_id ? ip->stride2 : ip->stride1;
        auto data_ptr = ip->data + (cam_id ? ip->height * ip->stride1 : 0);
        char *img_content = new char[ip->height * img_stride];
        memcpy(img_content, data_ptr, ip->height * img_stride);
        new_data = new rc_Data{ (rc_Sensor)(ip->header.sensor_id + cam_id), rc_SensorType::rc_SENSOR_TYPE_IMAGE,
            (rc_Timestamp)ip->header.time, rc_DataPath::rc_DATA_PATH_SLOW, {} };
        new_data->image = rc_ImageData{ (rc_Timestamp)ip->exposure_time_us, ip->width, ip->height, img_stride,
            (rc_ImageFormat)ip->format, img_content, NULL, NULL };
        break;
    }
    case packet_accelerometer: {
        auto *ip = (packet_accelerometer_t *)packet.get();
        new_data = new rc_Data{ (rc_Sensor)ip->header.sensor_id, rc_SensorType::rc_SENSOR_TYPE_ACCELEROMETER,
            (rc_Timestamp)ip->header.time, rc_DataPath::rc_DATA_PATH_SLOW,{} };
        new_data->acceleration_m__s2 = {{ ip->a[0], ip->a[1], ip->a[2] }};
        break;
    }
    case packet_gyroscope: {
        auto *ip = (packet_gyroscope_t *)packet.get();
        new_data = new rc_Data{ (rc_Sensor)ip->header.sensor_id, rc_SensorType::rc_SENSOR_TYPE_GYROSCOPE,
            (rc_Timestamp)ip->header.time, rc_DataPath::rc_DATA_PATH_SLOW,{} };
        new_data->angular_velocity_rad__s = {{ ip->w[0], ip->w[1], ip->w[2] }};
        break;
    }
    }
    return { new_data, delete_rc_data };
}

