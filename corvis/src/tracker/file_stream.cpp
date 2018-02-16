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

static void log_to_stderr(void *handle, rc_MessageLevel message_level, const char * message, size_t len) {
    cerr << message;
}

file_stream::file_stream(const char *name): track_output(new replay_output()) {
    const int buffer_bytes = 128 * 1024;
    buffer = std::unique_ptr<char[]>(new char[buffer_bytes]);
    sensor_file.rdbuf()->pubsetbuf(buffer.get(), buffer_bytes);
    sensor_file.open(name, ios::binary);
    rp_device = new replay_device();
    track_output->host = this;
    track_output->on_track_output = process_track_output;
    device_stream::pose_handle = track_output.get();
    device_stream::pose_callback = pose_data_callback;
    device_stream::message_callback = log_to_stderr;
    device_stream::map_load_callback = mem_load_callback;
    device_stream::save_callback = file_save_callback;
}

////////////////   H O S T    S T R E A M     ////////////////////////

bool file_stream::init_stream() {
    if (!sensor_file.is_open()) return false;
    sensor_file.seekg(0, ios::end);
    sensor_data_size = (float)sensor_file.tellg();
    sensor_file.seekg(0, ios::beg);
    rp_device->init(this);
    return !sensor_file.bad() && !sensor_file.eof();
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
            case packet_enable_features_output: { track_output->set_output_type(replay_output::output_mode::POSE_FEATURE); break; }
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
            case packet_enable_stage_callback: {
                if (host_stage_callback) {
                    device_stream::stage_callback = [](void *handle, rc_Stage stage) {
                        ((file_stream *)handle)->host_stage_callback(stage);
                    };
                    device_stream::stage_handle = this;
                }
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
        if (!(sts = !sensor_file.eof())) {
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

void file_stream::put_host_packet(rc_packet_t &&new_packet) {
    if (!new_packet) return;
    {
        lock_guard<mutex> lk(host_queue_mtx);
        host_queue.push_back(move(new_packet));
    }
    control_packet_cond.notify_one();
}

void file_stream::put_device_packet(const rc_packet_t &device_packet) {
    if (!device_packet) return;
    switch (get_packet_type(device_packet)) {
    case packet_timing_stat: tracking_stat.assign((const char*)device_packet->data);
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
