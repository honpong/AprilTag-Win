#include <string.h>
#include <algorithm>

#include "file_stream.h"
#include "resource.h"
#include "replay_device.h"

using namespace std;

static void file_save_callback(void *handle, const void *buffer, size_t length) {
    ofstream * out = (ofstream *)handle;
    if (length > 0) out->write((const char *)buffer, length);
}

static void process_track_output(const replay_output *output, const rc_Data *data) {
    output->host->host_data_callback(output, data);
}

static void process_stage(void *handle, rc_Tracker * tracker, const rc_Stage * stage) {
    const replay_output *output = (const replay_output *)handle;
    if (output->host->stage_callback)
        output->host->stage_callback(stage);
}

static void log_to_stderr(void *handle, rc_MessageLevel message_level, const char * message, size_t len) {
    file_stream * stream = (file_stream *)handle;
    cerr << stream->name << ": " << message;
}

file_stream::file_stream(const char *name) {
    sensor_file.rdbuf()->pubsetbuf(buffer.get(), file_buffer_bytes);
    sensor_file.open(name, ios::binary);
    if ((stream_sts = sensor_file.is_open())) {
        sensor_file.seekg(0, ios::end);
        sensor_data_size = (float)sensor_file.tellg();
        sensor_file.seekg(0, ios::beg);
        stream_sts = sensor_file.good();
    }
    rp_device = new replay_device();
    track_output[rc_DATA_PATH_FAST].host = this;
    track_output[rc_DATA_PATH_FAST].on_track_output = process_track_output;
    track_output[rc_DATA_PATH_SLOW].host = this;
    track_output[rc_DATA_PATH_SLOW].on_track_output = process_track_output;
    device_stream::name = name;
    device_stream::pose_handle = &track_output;
    device_stream::pose_callback = pose_data_callback;
    device_stream::message_handle = this;
    device_stream::message_callback = log_to_stderr;
    device_stream::map_load_callback = mem_load_callback;
    device_stream::save_callback = file_save_callback;
    device_stream::stage_handle = &track_output[rc_DATA_PATH_SLOW];
    device_stream::stage_callback = process_stage;
}

////////////////   H O S T    S T R E A M     ////////////////////////

bool file_stream::init_stream() {
    return (stream_sts = stream_sts && rp_device->init(this, { rc_create(), rc_destroy }));
}

bool file_stream::start_stream() {
    rp_device->start();
    return true;
}

////////////////   D E V I C E    S T R E A M     ////////////////////////

bool file_stream::read_header(packet_header_t *header, bool control_only) {
    bool sts = false;
    {
        unique_lock<mutex> lk(host_queue_mtx);
        if (control_only && host_queue.empty()) {
            control_packet_cond.wait(lk, [&] { return !host_queue.empty(); });
        }
        if (!host_queue.empty()) {
            packet = move(host_queue.front());
            host_queue.pop_front();
            switch (get_packet_type(packet)) {
            case packet_command_start: {
                enable_sensor = true;
                last_progress = sensor_clock::now();
                break;
            }
            case packet_enable_output_mode: {
                track_output[rc_DATA_PATH_SLOW].set_output_type(get_packet_item(packet));
                break;
            }
            case packet_load_map: {
                ifstream file_handle(string((const char *)packet->data), ifstream::ate | ios_base::binary);
                size_t map_size_bytes = file_handle.tellg();
                packet = packet_control_alloc(packet_load_map, NULL, map_size_bytes);
                file_handle.seekg(ios_base::beg);
                file_handle.read((char *)packet->data, map_size_bytes);
                stream_handle = make_unique<replay_output::bstream_buffer>(0, map_size_bytes, (char *)packet->data);
                device_stream::map_load_handle = stream_handle.get();
                break;
            }
            case packet_save_calibration:
            case packet_save_map: {
                save_file = ofstream((char *)packet->data, ios_base::binary);
                device_stream::save_handle = &save_file;
                break;
            }
            case packet_command_stop: {
                enable_sensor = false;
                break;
            }
            }
            if (packet) *header = packet->header;
            sts = true;
        }
    }

    if (!sts && enable_sensor && !control_only) { //reading sensor data
        sensor_file.read((char *)header, sizeof(packet_header_t));
        if (!(sts = !sensor_file.eof()) || header->bytes > max_packet_size) {
            enable_sensor = false;
            packet = packet_command_alloc(packet_command_stop);
        }
        else if ((sts = !sensor_file.bad())) {
            packet = rc_packet_t((packet_t *)malloc(header->bytes), free);
            packet->header = *header;
            sensor_file.read(reinterpret_cast<char *>(&packet->data[0]), packet->header.bytes - sizeof(packet_header_t));
            //update progress
            if ((sts = !sensor_file.bad() && !sensor_file.eof())) {
                bytes_dispatched += packet->header.bytes;
                packets_dispatched++;
                if (progress_callback) {// Update progress at most at 30Hz or if we are almost done
                    auto now = sensor_clock::now();
                    if (now - last_progress > chrono::milliseconds(33) || bytes_dispatched / sensor_data_size > 0.99) {
                        last_progress = now;
                        progress_callback(bytes_dispatched / sensor_data_size);
                    }
                }
            }
        }
    }
    return sts;
}

bool file_stream::put_host_packet(rc_packet_t &&new_packet) {
    if (!new_packet) return true; //skip empty packet in host_queue
    {
        lock_guard<mutex> lk(host_queue_mtx);
        host_queue.push_back(move(new_packet));
    }
    control_packet_cond.notify_one();
    return true;
}

void file_stream::put_device_packet(const rc_packet_t &device_packet) {
    if (!device_packet) return;
    switch (get_packet_type(device_packet)) {
    case packet_timing_stat: { tracking_stat.assign((const char*)device_packet->data); break; }
    case packet_storage_stat: { storage_stat = get_packet_item(device_packet); break; }
    case packet_camera_extrinsics: {
        memcpy(&camera_extrinsics[0], device_packet->data, 2 * sizeof(rc_Extrinsics));
        break;
    }
    case packet_command_end: { //ensure share poitner to tracker's features are cleared
        track_output[rc_DATA_PATH_SLOW].rc_resetFeatures();
        track_output[rc_DATA_PATH_FAST].rc_resetFeatures();
        break;
    }
    }
    {   //inform waiting host upon new packet from device
        lock_guard<mutex> lk(host_stream::wait_device_mtx);
        host_stream::arrived_type = get_packet_type(device_packet);
    }
    host_stream::device_response.notify_all();
}

file_stream::~file_stream() {
    if (rp_device) delete rp_device;
}
