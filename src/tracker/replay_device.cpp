#include <string.h>
#include <cinttypes>
#include <thread>
#include <regex>
#include "replay_device.h"
#include "file_stream.h"
#include "platform/sensor_clock.h"
#include "Trace.h"

using namespace std;

template <int by_x, int by_y>
static void scale_down_inplace_y8_by(uint8_t *image, int final_width, int final_height, int stride) {
    for (int y=0; y<final_height; y++)
        for (int x = 0; x <final_width; x++) {
            int sum = 0; // FIXME: by_x * by_y / 2;
            for (int i=0; i<by_y; i++)
                for (int j=0; j<by_x; j++)
                    sum += image[stride * (by_y*y + i) + by_x*x + j];
            image[stride * y + x] = sum / (by_x * by_y);
        }
}

template <int by_x, int by_y>
static void scale_down_inplace_z16_by(uint16_t *image, int final_width, int final_height, int stride) {
    for (int y=0; y<final_height; y++)
        for (int x = 0; x <final_width; x++) {
            int sum = 0, div = 0;
            for (int i=0; i<by_y; i++)
                for (int j=0; j<by_x; j++)
                    if (auto v = image[stride/sizeof(uint16_t) * (by_y*y + i) + by_x*x + j]) {
                        sum += v;
                        div++;
                    }
            image[stride/sizeof(uint16_t) * y + x] = div ? sum / div : 0;
        }
}

bool replay_device::init(device_stream *_stream_object, std::unique_ptr <rc_Tracker, decltype(&rc_destroy)> tracker_) {
    tracker = std::move(tracker_);
    if (!tracker) {
        printf("Error: failed to create tracker instance\n");
        return false;
    }
    if (!_stream_object) return false;
    stream = _stream_object;
    return true;
}

void replay_device::zero_bias() {
    for (rc_Sensor id = 0; true; id++) {
        rc_Extrinsics extrinsics;
        rc_AccelerometerIntrinsics intrinsics;
        if (!rc_describeAccelerometer(tracker.get(), id, &extrinsics, &intrinsics))
            break;
        for (auto &b : intrinsics.bias_m__s2.v) b = 0;
        for (auto &b : intrinsics.bias_variance_m2__s4.v) b = 1e-3f;
        rc_configureAccelerometer(tracker.get(), id, nullptr, &intrinsics);
    }
    for (rc_Sensor id = 0; true; id++) {
        rc_Extrinsics extrinsics;
        rc_GyroscopeIntrinsics intrinsics;
        if (!rc_describeGyroscope(tracker.get(), id, &extrinsics, &intrinsics))
            break;
        for (auto &b : intrinsics.bias_rad__s.v) b = 0;
        for (auto &b : intrinsics.bias_variance_rad2__s2.v) b = 1e-4f;
        rc_configureGyroscope(tracker.get(), id, &extrinsics, &intrinsics);
    }
}

void replay_device::setup_filter()
{
    if (stream->pose_callback) rc_setDataCallback(tracker.get(), stream->pose_callback, stream->pose_handle);
    if (stream->status_callback) rc_setStatusCallback(tracker.get(), stream->status_callback, tracker.get());
}

#ifdef MYRIAD2
#include <unistd.h>
#endif

static void device_sleep(chrono::microseconds time_micro) {
#ifdef MYRIAD2
    usleep(time_micro.count());
#else
    this_thread::sleep_for(time_micro);
#endif
}

void replay_device::process_data(rc_packet_t &phandle) {
    packet_t *packet = phandle.get();
    auto data_timestamp = sensor_clock::micros_to_tp(packet->header.time);
    auto now = sensor_clock::now();
    if (is_realtime && realtime_offset == chrono::microseconds(0))
        realtime_offset = now - data_timestamp;
    if (is_realtime && data_timestamp + realtime_offset - now > chrono::microseconds(0)) {
        device_sleep(data_timestamp + realtime_offset - now);
    }
    switch (get_packet_type(phandle))
    {
    case packet_calibration_json: break;
    case packet_camera: {
        int width, height, stride;
        char tmp[17];
        memcpy(tmp, packet->data, 16);
        tmp[16] = 0;
        stringstream parse(tmp);
        //pgm header is "P5 x y"
        parse.ignore(3, ' ') >> width >> height;
        stride = width;

        auto *ip = (packet_camera_t *)packet;
        if ((qvga && width == 640 && height == 480) ||
            (qres == 1 && width % 2 == 0 && height % 2 == 0))
            scale_down_inplace_y8_by<2, 2>(ip->data + 16, width /= 2, height /= 2, stride);
        else if (qres == 2 && width % 4 == 0 && height % 4 == 0)
            scale_down_inplace_y8_by<4, 4>(ip->data + 16, width /= 4, height /= 4, stride);

        rc_receiveImage(tracker.get(), 0, rc_FORMAT_GRAY8, ip->header.time, 33333,
            width, height, stride, ip->data + 16,
            [](void *packet) { free(packet); }, phandle.release());

        is_stepping = false;
        break;
    }
    case packet_image_with_depth: {
        auto *ip = (packet_image_with_depth_t *)packet;

        ip->header.sensor_id = 1; // ref count
        if (use_depth && ip->depth_height && ip->depth_width) {
            ip->header.sensor_id++; // ref count
            uint16_t *depth_image = (uint16_t*)(ip->data + ip->width * ip->height);
            int depth_width = ip->depth_width, depth_height = ip->depth_height, depth_stride = sizeof(uint16_t) * ip->depth_width;
            if ((qvga && depth_width == 640 && depth_height == 480) ||
                (qres == 1 && depth_width % 2 == 0 && depth_height % 2 == 0))
                scale_down_inplace_z16_by<2, 2>(depth_image, depth_width /= 2, depth_height /= 2, depth_stride);
            else if (qres == 2 && depth_width % 4 == 0 && depth_height % 4 == 0)
                scale_down_inplace_z16_by<4, 4>(depth_image, depth_width /= 4, depth_height /= 4, depth_stride);
            rc_receiveImage(tracker.get(), 0, rc_FORMAT_DEPTH16, ip->header.time, 0,
                depth_width, depth_height, depth_stride, depth_image,
                [](void *packet) { if (!--((packet_header_t *)packet)->sensor_id) free(packet); }, packet);
        }

        auto *image = ip->data;
        int width = ip->width, height = ip->height, stride = ip->width;
        if ((qvga && width == 640 && height == 480) ||
            (qres == 1 && width % 2 == 0 && height % 2 == 0))
            scale_down_inplace_y8_by<2, 2>(image, width /= 2, height /= 2, stride);
        else if (qres == 2 && width % 4 == 0 && height % 4 == 0)
            scale_down_inplace_y8_by<4, 4>(image, width /= 4, height /= 4, stride);

        rc_receiveImage(tracker.get(), 0, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
            width, height, stride, image,
            [](void *packet) { if (!--((packet_header_t *)packet)->sensor_id) free(packet); }, phandle.release());

        is_stepping = false;
        break;
    }
    case packet_image_raw: {
        auto *ip = (packet_image_raw_t *)packet;
        START_EVENT(EV_SF_REC_IMAGE, 0);
        if (ip->format == rc_FORMAT_GRAY8) {
            if ((qvga && ip->width == 640 && ip->height == 480) ||
                (qres == 1 && ip->width % 2 == 0 && ip->height % 2 == 0))
                scale_down_inplace_y8_by<2, 2>(ip->data, ip->width /= 2, ip->height /= 2, ip->stride);
            else if (qres == 2 && ip->width % 4 == 0 && ip->height % 4 == 0)
                scale_down_inplace_y8_by<4, 4>(ip->data, ip->width /= 4, ip->height /= 4, ip->stride);
            rc_receiveImage(tracker.get(), packet->header.sensor_id, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
                ip->width, ip->height, ip->stride, ip->data,
                [](void *packet) { free(packet); }, phandle.release());
        }
        else if (use_depth && ip->format == rc_FORMAT_DEPTH16) {
            if ((qvga && ip->width == 640 && ip->height == 480) ||
                (qres == 1 && ip->width % 2 == 0 && ip->height % 2 == 0))
                scale_down_inplace_z16_by<2, 2>((uint16_t*)ip->data, ip->width /= 2, ip->height /= 2, ip->stride);
            else if (qres == 2 && ip->width % 4 == 0 && ip->height % 4 == 0)
                scale_down_inplace_z16_by<4, 4>((uint16_t*)ip->data, ip->width /= 4, ip->height /= 4, ip->stride);
            rc_receiveImage(tracker.get(), packet->header.sensor_id, rc_FORMAT_DEPTH16, ip->header.time, ip->exposure_time_us,
                ip->width, ip->height, ip->stride, ip->data,
                [](void *packet) { free(packet); }, phandle.release());
        }
        END_EVENT(EV_SF_REC_IMAGE, 0);
        is_stepping = false;
        break;

    }
    case packet_stereo_raw: {
        if (!stereo_configured) {
            rc_configureStereo(tracker.get(), 0, 1);
            stereo_configured = true;
        }
        START_EVENT(EV_SF_REC_STEREO, 0);
        auto *ip = (packet_stereo_raw_t *)packet;
        if (ip->format == rc_FORMAT_GRAY8) {
            auto *data2 = ip->data + ip->stride1*ip->height;
            if ((qvga && ip->width == 640 && ip->height == 480) ||
                (qres == 1 && ip->width % 2 == 0 && ip->height % 2 == 0)) {
                scale_down_inplace_y8_by<2, 2>(ip->data,  ip->width /= 2,   ip->height /= 2, ip->stride1);
                scale_down_inplace_y8_by<2, 2>(    data2, ip->width,        ip->height,      ip->stride2);
            }
            else if (qres == 2 && ip->width % 4 == 0 && ip->height % 4 == 0) {
                scale_down_inplace_y8_by<4, 4>(ip->data,  ip->width /= 4, ip->height /= 4, ip->stride1);
                scale_down_inplace_y8_by<4, 4>(    data2, ip->width,      ip->height,      ip->stride2);
            }
            rc_receiveStereo(tracker.get(), packet->header.sensor_id, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
                ip->width, ip->height, ip->stride1, ip->stride2, ip->data, (uint8_t *)data2,
                [](void *packet) { free(packet); }, phandle.release());
        }
        END_EVENT(EV_SF_REC_STEREO, 0);
        is_stepping = false;
        break;
    }
    case packet_accelerometer: {
        START_EVENT(EV_SF_REC_ACCEL, 0);
        auto *p = (packet_accelerometer_t *)packet;
        const rc_Vector acceleration_m__s2 = {{ p->a[0], p->a[1], p->a[2] }};
        rc_receiveAccelerometer(tracker.get(), packet->header.sensor_id, packet->header.time, acceleration_m__s2);
        END_EVENT(EV_SF_REC_ACCEL, 0);
        break;
    }
    case packet_gyroscope: {
        START_EVENT(EV_SF_REC_GYRO, 0);
        auto *p = (packet_gyroscope_t *)packet;
        const rc_Vector angular_velocity_rad__s = {{ p->w[0], p->w[1], p->w[2] }};
        rc_receiveGyro(tracker.get(), packet->header.sensor_id, packet->header.time, angular_velocity_rad__s);
        END_EVENT(EV_SF_REC_GYRO, 0);
        break;
    }
    case packet_imu: {
        auto *imu = (packet_imu_t *)packet;
        const rc_Vector acceleration_m__s2 = { {imu->a[0], imu->a[1], imu->a[2]} }, angular_velocity_rad__s = { {imu->w[0], imu->w[1], imu->w[2]} };
        rc_receiveAccelerometer(tracker.get(), packet->header.sensor_id, packet->header.time, acceleration_m__s2);
        rc_receiveGyro(tracker.get(), packet->header.sensor_id, packet->header.time, angular_velocity_rad__s);
        break;
    }
    case packet_thermometer:
    {
        auto *thermometer = (packet_thermometer_t *)packet;
        rc_receiveTemperature(tracker.get(), packet->header.sensor_id, packet->header.time, thermometer->temperature_C);
        break;
    }
    case packet_arrival_time: {
        // ignore arrival_time packets for now
        break;
    }
    case packet_controller_physical_info: {
         // ignore controller_physical_info packets for now
        break;
    }
    case packet_exposure: {
         // ignore exposure packets for now
        break;
    }
    case packet_velocimeter:
    {
        if (!use_odometry)
            break;
        auto velocimeter = (packet_velocimeter_t *)packet;
        rc_receiveVelocimeter(tracker.get(), velocimeter->header.sensor_id, velocimeter->header.time, rc_Vector{ { velocimeter->v[0],velocimeter->v[1], velocimeter->v[2] } });
        break;
    }
    case packet_diff_velocimeter:
    {
        if (!use_odometry)
            break;
        START_EVENT(EV_SF_REC_VELO, 0);
        auto diff_velocity = (packet_diff_velocimeter_t *)packet;
        rc_receiveVelocimeter(tracker.get(), diff_velocity->header.sensor_id, diff_velocity->header.time, rc_Vector{ { diff_velocity->v[0],0,0 } });
        rc_receiveVelocimeter(tracker.get(), diff_velocity->header.sensor_id + 1, diff_velocity->header.time, rc_Vector{ { diff_velocity->v[1],0,0 } });
        END_EVENT(EV_SF_REC_VELO, diff_velocity->header.sensor_id);
        break;
    }
    case packet_filter_control: {
        if (packet->header.sensor_id == 0)
        {
            printf("All data received, exiting\n");
            is_running = false;
        }
        break;
    }
    default:
        printf("Unrecognized data type %d\n", packet->header.type);
        break;
    }
    stream->device_ack(sensor_clock::tp_to_micros(data_timestamp));
}

void replay_device::process_control(const packet_control_t *packet) {
    switch (packet->header.control_type) {
    case packet_enable_realtime: { is_realtime = true; break; }
    case packet_enable_qvga: { qvga = true; break; }
    case packet_enable_qres: { qres = get_packet_item(packet); break; }
    case packet_enable_async: { async = is_realtime = true; break; }
    case packet_enable_no_depth: { use_depth = false; break; }
    case packet_enable_fast_path: { fast_path = true; break; }
    case packet_enable_dynamic_calibration: { dynamic_calibration = true; break; }
    case packet_enable_zero_biases: {
        zero_bias();
        to_zero_biases = true; // used if load calibration happens after
        break; 
    }
    case packet_enable_odometry: { use_odometry = true; break; }
    case packet_enable_mesg_level: { message_level = get_packet_item(packet); break; }
    case packet_enable_mapping: { rc_startMapping(tracker.get(), false, get_packet_item(packet)); break; }
    case packet_set_queue_strategy: {
        queue_strategy = get_packet_item(packet);
        strategy_override = true;
        break;
    }
    case packet_enable_relocalization: { rc_startMapping(tracker.get(), true, true); break; }
    case packet_command_start: {
        if (stream->message_callback) rc_setMessageCallback(tracker.get(), stream->message_callback, stream->message_handle, message_level);
        rc_configureQueueStrategy(tracker.get(), (strategy_override) ? queue_strategy :
                                  (async ? rc_QUEUE_MINIMIZE_LATENCY : queue_strategy));
        rc_startTracker(tracker.get(),
                        (async ? rc_RUN_ASYNCHRONOUS : rc_RUN_SYNCHRONOUS) |
                        (fast_path ? rc_RUN_FAST_PATH : rc_RUN_NO_FAST_PATH) |
                        (dynamic_calibration? rc_RUN_DYNAMIC_CALIBRATION : rc_RUN_STATIC_CALIBRATION));
        break;
    }
    case packet_load_map: {
        if (rc_loadMap(tracker.get(), stream->map_load_callback, stream->map_load_handle))
            printf("Done loading relocalization map.\n");
        else
            printf("Error: failed to load map file!\n");
        break;
    }
    case packet_save_map: {
        if (stream->save_callback) {
            rc_saveMap(tracker.get(), [](void *handle, const void *buffer, size_t length) {
                device_stream *stream = (device_stream *)handle;
                if (length) {
                    stream->save_callback(stream->save_handle, buffer, length);
                } else {
                    stream->put_device_packet(packet_command_alloc(packet_save_end));
                }
            }, stream);
        }
        break;
    }
    case packet_load_calibration_bin: {
        if (!rc_setCalibrationTM2(tracker.get(), (const char *)packet->data, packet->header.bytes))
            fprintf(stderr, "Error: failed to load binary calibration...\n");
        else if (to_zero_biases)
            zero_bias();
        break;
    }
    case packet_load_calibration: {
        if (!rc_setCalibration(tracker.get(), (const char *)packet->data))
            fprintf(stderr, "Error: failed to load JSON calibration...\n");
        else if (to_zero_biases)
            zero_bias();
        break;
    }
    case packet_calibration_json_extra: {
        if (!rc_appendCalibration(tracker.get(), (const char *)packet->data))
            fprintf(stderr, "Error: failed to load JSON velocimeter calibration...\n");
        break;
    }
    case packet_save_calibration: {
        if (stream->save_callback) {
            const char * buffer = nullptr;
            size_t bytes = rc_getCalibration(tracker.get(), &buffer);
            stream->save_callback(stream->save_handle, buffer, bytes);
            stream->put_device_packet(packet_command_alloc(packet_save_end));
        }
        break;
    }
    case packet_delay_start: { delay_start = get_packet_item(packet); break; }
    case packet_command_step: { is_paused = is_stepping = true; break; }
    case packet_command_next_pause: {
        rc_Timestamp pause_time = get_packet_item(packet);
        next_pause = pause_time;
        break;
    }
    case packet_command_toggle_pause: { is_paused = !is_paused; break; }
    case packet_command_reset: {
        fprintf(stderr, "Resetting...");
        rc_stopTracker(tracker.get());
        rc_startTracker(tracker.get(),
                        (async ? rc_RUN_ASYNCHRONOUS : rc_RUN_SYNCHRONOUS) |
                        (fast_path ? rc_RUN_FAST_PATH : rc_RUN_NO_FAST_PATH) |
                        (dynamic_calibration ? rc_RUN_DYNAMIC_CALIBRATION : rc_RUN_STATIC_CALIBRATION));
        fprintf(stderr, "done\n");
        break;
    }
    case packet_timing_stat: {
        string timing_stat(rc_getTimingStats(tracker.get()));
        stream->put_device_packet(packet_control_alloc(packet_timing_stat, timing_stat.c_str(), timing_stat.size() + 1));
        break;
    }
    case packet_storage_stat: {
        rc_StorageStats storage_stat = rc_getStorageStats(tracker.get());
        stream->put_device_packet(packet_single_control_alloc(packet_storage_stat, storage_stat));
        break;
    }
    case packet_set_stage: { set_stage(); break; }
    case packet_command_stop: {
        rc_stopTracker(tracker.get());
        stream->put_device_packet(packet_command_alloc(packet_command_stop));
        break;
    }
    case packet_camera_extrinsics: {
        rc_Extrinsics extrinsics[2] = { rc_Extrinsics{} };
        rc_describeCamera(tracker.get(), 0, rc_FORMAT_GRAY8, &extrinsics[0], nullptr);
        rc_describeCamera(tracker.get(), 1, rc_FORMAT_GRAY8, &extrinsics[1], nullptr);
        stream->put_device_packet(packet_control_alloc(packet_camera_extrinsics, (char *)&extrinsics, 2 * sizeof(rc_Extrinsics)));
        break;
    }
    case packet_command_end: { is_running = false; }
    }
}

void replay_device::start() {
    if (!stream) return;
    setup_filter();

    uint64_t first_data_timestamp = 0;
    packet_header_t header = { 0 };
    is_running = stream->read_header(&header, true);
    auto phandle = stream->get_host_packet();
    while (is_running) {
        if (next_pause && next_pause <= header.time) {
            fprintf(stderr, "Paused at %" PRIu64 "\n", header.time);
            next_pause = 0;
            is_paused = true;
        }
        {
            auto start_pause = sensor_clock::now();
            auto finish_pause = start_pause;
            while (is_paused && !is_stepping && is_running) { //wait until next toggled pause
                if (stream->read_header(&header, true)) { //read only control type
                    auto phandle = stream->get_host_packet();
                    process_control((packet_control_t *)phandle.get());
                }
                finish_pause = sensor_clock::now();
            }
            realtime_offset += finish_pause - start_pause;
        }

        if (phandle) {
            if (phandle->header.type == packet_control)
                process_control((packet_control_t *)phandle.get());
            else {
                if (delay_start && !first_data_timestamp)
                    first_data_timestamp = header.time;
                if (header.time >= first_data_timestamp + delay_start) {
                    process_data(phandle);
                }
            }
        }
          ////waiting for next control packet if late
        if (is_running && !stream->read_header(&header)) {
            auto start_pause = sensor_clock::now();
            device_sleep(chrono::microseconds(500));
            realtime_offset += sensor_clock::now() - start_pause;
        }
        phandle = stream->get_host_packet();
    } //while
    rc_stopTracker(tracker.get());
    tracker = nullptr;
    stream->put_device_packet(packet_command_alloc(packet_command_end)); // signal any usb waiting call to end
}

void replay_device::set_stage() {
    static int x; std::string name = "stage" + std::to_string(x++);
    rc_PoseTime pose = rc_getPose(tracker.get(), nullptr, nullptr, rc_DATA_PATH_SLOW);
    if (rc_setStage(tracker.get(), name.c_str(), pose.pose_m))
        printf("created --stage=%s=%" PRId64 "\n", name.c_str(), pose.time_us);
    else
        printf("Error: can't create stage; has the filter converged?\n");
}
