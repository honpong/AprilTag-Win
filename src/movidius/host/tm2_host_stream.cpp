#include <signal.h>  // capture sigint
#include "tm2_host_stream.h"
#include "usb_host.h"

using namespace std;
using namespace std::chrono;

uint64_t bulk_packet_size = 20000;

template<uint8_t packet_type>
static bool async_stream_file(const char* filename) {
    ifstream file_handle(filename, ios_base::binary);
    const string file_content((istreambuf_iterator<char>(file_handle)), istreambuf_iterator<char>());
    uint64_t total_bytes = file_content.size();

    rc_packet_t transfer_packet = packet_control_alloc(packet_type, (const char*)&total_bytes, sizeof(uint64_t));
    usb_write_packet(transfer_packet); //first packet of total transfer size

    size_t transferred_bytes = 0;
    transfer_packet = rc_packet_t((packet_t *)malloc(bulk_packet_size + sizeof(packet_header_t)), free);
    while (transferred_bytes < total_bytes) {
        size_t transfer_size = std::min(total_bytes - transferred_bytes, bulk_packet_size);
        set_control_packet((packet_control_t *)transfer_packet.get(), packet_type,
            file_content.c_str() + transferred_bytes, transfer_size);
        usb_write_packet(transfer_packet);
        transferred_bytes += transfer_size;
    }
    return true;
}

template<uint8_t packet_type>
static bool sync_stream_file(const char *filename) {
    async_stream_file<packet_type>(filename);
    rc_packet_t ack_packet((packet_t*)malloc(sizeof(packet_t)), free);
    usb_read_interrupt_packet(ack_packet);
    if (get_packet_type(ack_packet) != packet_ack) {
        printf("Error: failed to stream of type %d from file %s.\n", packet_type, filename);
        return false;
    }
    return true;
}

static void save_stream_to_file(const packet_t* save_packet, ofstream &stream) {
    if (save_packet->header.bytes > sizeof(packet_header_t)) {
        stream.write((const char *)save_packet->data, save_packet->header.bytes - sizeof(packet_header_t));
    }
    else //empty body signals end of saving
        stream.close();
}

tm2_host_stream::tm2_host_stream(const char*filename) {
    sensor_file.rdbuf()->pubsetbuf(buffer.get(), file_buffer_bytes);
    sensor_file.open(filename, ios::binary);
    if ((stream_sts = sensor_file.is_open())) {
        sensor_file.seekg(0, ios::end);
        sensor_data_size = (size_t)sensor_file.tellg();
        sensor_file.seekg(0, ios::beg);
        stream_sts = sensor_file.good();
    }
};

void shutdown(int _unused = 0) {
    usb_shutdown();
}

bool tm2_host_stream::init_stream() {
    is_usb_ok = stream_sts && usb_init();
    signal(SIGINT, shutdown);
    if (is_usb_ok) usb_send_control(CONTROL_MESSAGE_START, 0, 0, 0);
    return is_usb_ok;
}

bool tm2_host_stream::start_stream() {
    //separate thread to read usb data from device
    thread_6dof_output = thread([this] {
        size_t packet_size = track_output.get_buffer_size();
        rc_packet_t output_pkt((packet_t *)malloc(packet_size), free);
        output_pkt->header.bytes = packet_size;
        while (1) {
            //use different transfer mode than other packet types.
            bool is_read = usb_read_interrupt_packet(output_pkt); //require same size packets
            if (!is_read || get_packet_type(output_pkt) == packet_command_end) break;
            else if (host_data_callback){
                track_output.set((char *)output_pkt.get(), output_pkt->header.bytes);
                rc_Data *passed_data = nullptr;
                {
                    lock_guard<mutex> lk(image_queue_mtx);
                    auto found_itr = find_if(sensor_queue.begin(), sensor_queue.end(),
                        [&](auto &ele) {
                            return (ele->time_us == track_output.sensor_time_us) && (ele->type == track_output.sensor_type) &&
                            ((ele->type == rc_SENSOR_TYPE_IMAGE) ? (track_output.sensor_id == ele->id) : true);
                        });
                    if (found_itr != sensor_queue.end()) {
                        passed_data = found_itr->get();
                        passed_data->path = track_output.data_path;
                    }
                }

                if (passed_data) host_data_callback(&track_output, passed_data);

                if (track_output.sensor_type == rc_SENSOR_TYPE_IMAGE || track_output.sensor_type == rc_SENSOR_TYPE_STEREO)
                {
                    lock_guard<mutex> lk(image_queue_mtx);
                    sensor_queue.remove_if([&](auto &ele) { return ele->time_us < track_output.sensor_time_us; }); // keep current
                }
            }
        }
    });

    thread_receive_device = thread([this] {
        rc_packet_t output_pkt( nullptr, free );
        replay_output track_output;
        while (1) {
            output_pkt = usb_read_packet();
            if (!output_pkt || get_packet_type(output_pkt) == packet_command_end) break;
            else {
                switch (get_packet_type(output_pkt)) {
                case packet_save_data: { save_stream_to_file(output_pkt.get(), save_file); break; }
                case packet_timing_stat: { tracking_stat.assign((const char*)output_pkt->data); break; }
                case packet_storage_stat: { memcpy(&storage_stat, output_pkt->data, sizeof(rc_StorageStats)); break; }
                case packet_camera_extrinsics: {
                    memcpy(&camera_extrinsics[0], output_pkt->data, 2 * sizeof(rc_Extrinsics));
                    break;
                }
                case packet_rc_relocalization: {
                    if (relocalization_callback) {
                        const packet_rc_relocalization_t* reloc_pkt = (packet_rc_relocalization_t*)output_pkt.get();
                        rc_Relocalization relocalization = { static_cast<rc_Timestamp>(reloc_pkt->timestamp), static_cast<enum rc_SessionId>(reloc_pkt->sessionid) };
                        relocalization_callback(&relocalization);
                    }
                    break;
                }
                }
                {//inform waiting host upon new packet from device
                    lock_guard<mutex> lk(host_stream::wait_device_mtx);
                    host_stream::arrived_type = get_packet_type(output_pkt);
                }
                host_stream::device_response.notify_all();
            }
        }
    });
    //streaming out sensor data.
    auto last_progress = high_resolution_clock::now();
    bool sts = stream_sts;
    rc_packet_t cur_packet(nullptr, free);
    packet_header_t header;
    while (enable_sensor && sts) {
        sensor_file.read((char *)&header, sizeof(packet_header_t));
        sts = !sensor_file.eof() && header.bytes < max_packet_size;
        if ((sts = sts && !sensor_file.bad())) {
            cur_packet = rc_packet_t((packet_t *)malloc(header.bytes), free);
            cur_packet->header = header;
            sensor_file.read(reinterpret_cast<char *>(&cur_packet->data[0]),
                cur_packet->header.bytes - sizeof(packet_header_t));
            if ((sts = !sensor_file.bad() && !sensor_file.eof())) {
                bytes_dispatched += cur_packet->header.bytes;
                packets_dispatched++;
                {
                    lock_guard<mutex> lk(image_queue_mtx);
                    auto data_rc_format = create_rc_Data(cur_packet);
                    if (data_rc_format) {
                        sensor_queue.emplace_back(move(data_rc_format));
                        if (get_packet_type(cur_packet) == packet_stereo_raw) //queue second single image
                            sensor_queue.emplace_back(create_rc_Data(cur_packet, 1));
                    }
                }
                enable_sensor = enable_sensor && put_host_packet(move(cur_packet)); //send sensor packets to device
                if (progress_callback) {  // Update progress at most at 30Hz or if we are almost done
                    auto now = high_resolution_clock::now();
                    if (duration<double, milli>(now - last_progress) > milliseconds(33) ||
                        (float)bytes_dispatched / sensor_data_size > 0.99) {
                        last_progress = now;
                        progress_callback((float)bytes_dispatched / sensor_data_size);
                    }
                }
                if (usb_sync) host_stream::wait_device_packet({ packet_sensor_ack });
            }
        }
        if (!sts) put_host_packet(packet_command_alloc(packet_command_stop));
    }
    if (thread_6dof_output.joinable()) thread_6dof_output.join();
    if (thread_receive_device.joinable()) thread_receive_device.join();
    usb_shutdown();
    return sts;
}

bool tm2_host_stream::put_host_packet(rc_packet_t &&post_packet) {
    std::lock_guard<std::mutex> lk(put_mutex);
    if (!is_usb_ok || !post_packet || stop_host_sending) return false;

    bool write_packet = true;
    packet = move(post_packet); //free prior packet
    switch (get_packet_type(packet)) {
    case packet_load_map: {
        async_stream_file<packet_load_map>((const char*)packet->data);
        write_packet = false;
        break;
    }
    case packet_save_calibration:
    case packet_save_map: {
        if (save_file.is_open()) save_file.close();
        save_file.open((const char*)packet->data, ios_base::binary);
        break;
    }
    case packet_enable_output_mode: {
        track_output.set_output_type(get_packet_item(packet));
        break;
    }
    case packet_enable_usb_sync: { usb_sync = true; break; }
    case packet_command_end: stop_host_sending = true;
    case packet_command_stop: enable_sensor = false;
    }

    if (write_packet)
        if (!usb_write_packet(packet)) is_usb_ok = false;

    if (!is_usb_ok) {
        {//inform waiting host to stop waiting
            lock_guard<mutex> wait_lk(host_stream::wait_device_mtx);
            host_stream::arrived_type = packet_invalid_usb;
        }
        host_stream::device_response.notify_all();
    }
    return is_usb_ok;
}

tm2_host_stream::~tm2_host_stream() {
    usb_shutdown();
}
