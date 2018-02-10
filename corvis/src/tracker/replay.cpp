//
//  replay.cpp
//
//  Created by Eagle Jones on 4/8/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "replay.h"

#include <string.h>
#include <regex>
#include <unordered_map>
#include "packet.h"

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

void log_to_stderr(void *handle, rc_MessageLevel message_level, const char * message, size_t len)
{
    std::cerr << message;
}

replay::replay(bool start_paused): is_paused(start_paused)
{
    tracker = rc_create();
}

bool replay::open(const char *name)
{
    file.open(name, ios::binary);
    if(!file.is_open())
    {
        cerr << "Couldn't open file " << name << " for reading.\n";
        return false;
    }
    file.seekg(0, ios::end);
    auto end = file.tellg();
    file.seekg (0, ios::beg);
    auto begin = file.tellg();
    size = end - begin;
    return true;
}

void replay::zero_biases()
{
    for (rc_Sensor id = 0; true; id++) {
        rc_Extrinsics extrinsics;
        rc_AccelerometerIntrinsics intrinsics;
        if (!rc_describeAccelerometer(tracker, id, &extrinsics, &intrinsics))
            break;
        for (auto &b : intrinsics.bias_m__s2.v) b = 0;
        for (auto &b : intrinsics.bias_variance_m2__s4.v) b = 1e-3f;
        rc_configureAccelerometer(tracker, id, nullptr, &intrinsics);
    }
    for (rc_Sensor id = 0; true; id++) {
        rc_Extrinsics extrinsics;
        rc_GyroscopeIntrinsics intrinsics;
        if (!rc_describeGyroscope(tracker, id, &extrinsics, &intrinsics))
            break;
        for (auto &b : intrinsics.bias_rad__s.v) b = 0;
        for (auto &b : intrinsics.bias_variance_rad2__s2.v) b = 1e-4f;
        rc_configureGyroscope(tracker, id, &extrinsics, &intrinsics);
    }
}

bool replay::load_internal_calibration(const std::string &filename)
{
    ifstream rc(filename);
    packet_header_t header = {};
    rc.read((char *)&header, sizeof(header));
    if (!rc || header.bytes <= sizeof(header) || header.type != packet_calibration_json)
        return false;
    std::string json;
    json.resize(header.bytes - sizeof(header));
    rc.read(&json[0], header.bytes - sizeof(header));
    if (!rc || !rc_setCalibration(tracker, json.c_str()))
        return false;
    calibration_file = filename + ".json"; // avoid the default save file being the rc file itself
    return true;
}

bool replay::load_calibration(const std::string &filename)
{
    ifstream file_handle(filename);
    if(file_handle.fail())
        return false;

    string json((istreambuf_iterator<char>(file_handle)), istreambuf_iterator<char>());

    if(!rc_setCalibration(tracker, json.c_str()))
        return false;

    calibration_file = filename;
    return true;
}

bool replay::save_calibration(const std::string &filename)
{
    const char * buffer = nullptr;
    size_t bytes = rc_getCalibration(tracker, &buffer);

    std::string json(buffer, bytes);
    std::ofstream out(filename);
    if(!out) return false;
    out << json;

    return true;
}

bool replay::set_calibration_from_filename(const char *filename)
{
    string fn(filename), json;
    if(!load_calibration(json = fn + ".json")) {
        auto found = fn.find_last_of("/\\");
        string path = fn.substr(0, found+1);
        if(!load_internal_calibration(filename) &&
           !load_calibration(json = path + "calibration.json"))
            return false;
    }
    return true;
}

void replay::set_relative_pose(const sensor_clock::time_point & timestamp, const tpose & pose)
{
    if(reference_seq) reference_seq->set_relative_pose(timestamp, pose);
}

bool replay::get_reference_pose(const sensor_clock::time_point & timestamp, tpose & pose_out)
{
    return reference_seq && reference_seq->get_pose(timestamp, pose_out);
}

bool replay::set_reference_from_filename(const string &filename)
{
    return load_reference_from_pose_file(filename + ".tum") ||
           load_reference_from_pose_file(filename + ".pose") ||
           load_reference_from_pose_file(filename + ".vicon") ||
           find_reference_in_filename(filename);
}

bool replay::load_reference_from_pose_file(const string &filename)
{
    unique_ptr<tpose_sequence> seq = make_unique<tpose_sequence>();
    if (seq->load_from_file(filename)) {
        reference_path_length = seq->get_path_length();
        reference_length = seq->get_length();
        reference_seq = move(seq);
        return true;
    }
    return false;
}

static bool find_prefixed_number(const std::string in, const std::string &prefix, double &n)
{
    smatch m; if (!regex_search(in, m, regex(prefix + "(\\d+(?:\\.\\d*)?)"))) return false;
    n = std::stod(m[1]);
    return true;
}

bool replay::find_reference_in_filename(const string &filename)
{
    bool PL = find_prefixed_number(filename, "_PL", reference_path_length); reference_path_length /= 100;
    bool L = find_prefixed_number(filename, "_L", reference_length); reference_length /= 100;
    return PL | L;
}

void replay::setup_filter()
{
    if(data_callback)
    {
        rc_setDataCallback(tracker, [](void *handle, rc_Tracker * tracker, const rc_Data * data) {
            replay * this_replay = (replay *)handle;
            this_replay->data_callback(tracker, data);
        }, this);
    }
    if(stage_callback)
    {
        rc_setStageCallback(tracker, [](void *handle, const char *description, const rc_Pose pose_m) {
            replay * this_replay = (replay *)handle;
            this_replay->stage_callback(description, pose_m);
        }, this);
    }
    rc_setMessageCallback(tracker, log_to_stderr, nullptr, message_level);
    rc_startTracker(tracker, (async ? rc_RUN_ASYNCHRONOUS : rc_RUN_SYNCHRONOUS) | (fast_path ? rc_RUN_FAST_PATH : rc_RUN_NO_FAST_PATH));
}

void replay::start(string map_filename)
{
    setup_filter();
    if (!map_filename.empty())
        load_map(map_filename);
    is_running = true;
    path_length = 0;
    length = 0;
    packets_dispatched = 0;
    bytes_dispatched = 0;
    bool is_stereo = false;

    packet_header_t header;
    file.read((char *)&header, 16);
    if(file.bad() || file.eof()) is_running = false;

    auto first_timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
    auto start_time = sensor_clock::now();
    auto now = start_time;
    auto last_progress = now;
    auto realtime_offset = now - first_timestamp;
    if(!is_realtime)
        realtime_offset = std::chrono::microseconds(0);

    while (is_running) {
        if(next_pause && next_pause <= header.time) {
            fprintf(stderr, "Paused at %" PRIu64 "\n", header.time);
            next_pause = 0;
            is_paused = true;
        }
        auto start_pause = sensor_clock::now();
        auto finish_pause = start_pause;
        while(is_paused  && !is_stepping && is_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            finish_pause = sensor_clock::now();
        }
        realtime_offset += finish_pause - start_pause;

        if (should_reset.exchange(false)) {
            fprintf(stderr, "Resetting...");
            rc_stopTracker(tracker);
            rc_startTracker(tracker, (async ? rc_RUN_ASYNCHRONOUS : rc_RUN_SYNCHRONOUS) | (fast_path ? rc_RUN_FAST_PATH : rc_RUN_NO_FAST_PATH));
            fprintf(stderr, "done\n");
        }

        auto phandle = std::unique_ptr<void, void(*)(void *)>(malloc(header.bytes), free);
        auto packet = (packet_t *)phandle.get();
        packet->header = header;
        
        file.read((char *)packet->data, header.bytes - 16);
        if(file.bad() || file.eof())
        {
            is_running = false;
        }
        else
        {
            auto data_timestamp = sensor_clock::micros_to_tp(header.time);
            auto timestamp = data_timestamp + realtime_offset;
            now = sensor_clock::now();
            if(is_realtime && timestamp - now > std::chrono::seconds(1)) {
                auto gap = std::chrono::duration_cast<std::chrono::microseconds>(timestamp - now);
                fprintf(stderr, "Warning: skipping a %f second gap\n", gap.count()/1.e6);
                realtime_offset -= (timestamp - now);
                timestamp -= realtime_offset;
            }
            if(is_realtime && timestamp - now > std::chrono::microseconds(0))
                std::this_thread::sleep_for(timestamp - now);

            switch(header.type)
            {
                case packet_calibration_json:
                    break;
                case packet_camera:
                {
                    int width, height, stride;
                    char tmp[17];
                    memcpy(tmp, packet->data, 16);
                    tmp[16] = 0;
                    std::stringstream parse(tmp);
                    //pgm header is "P5 x y"
                    parse.ignore(3, ' ') >> width >> height;
                    stride = width;

                    packet_camera_t *ip = (packet_camera_t *)packet;
                    if ((qvga && width == 640 && height == 480) ||
                        (qres == 1 && width % 2 == 0 && height % 2 == 0))
                        scale_down_inplace_y8_by<2,2>(ip->data + 16, width /= 2, height /= 2, stride);
                    else if (qres == 2 && width % 4 == 0 && height % 4 == 0)
                        scale_down_inplace_y8_by<4,4>(ip->data + 16, width /= 4, height /= 4, stride);

                    if(image_decimate && data_timestamp < last_image) break;
                    if(last_image == sensor_clock::time_point()) last_image = data_timestamp;
                    last_image += image_interval;

                    rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, ip->header.time, 33333,
                                    width, height, stride, ip->data + 16,
                                    [](void *packet) { free(packet); }, phandle.release());

                    is_stepping = false;
                    break;
                }
                case packet_image_with_depth:
                {
                    packet_image_with_depth_t *ip = (packet_image_with_depth_t *)packet;

                    if(image_decimate && data_timestamp < last_image) break;

                    ip->header.sensor_id = 1; // ref count
                    if (use_depth && ip->depth_height && ip->depth_width) {
                        ip->header.sensor_id++; // ref count
                        uint16_t *depth_image = (uint16_t*)(ip->data + ip->width * ip->height);
                        int depth_width = ip->depth_width, depth_height = ip->depth_height, depth_stride = sizeof(uint16_t) * ip->depth_width;
                        if ((qvga && depth_width == 640 && depth_height == 480) ||
                            (qres == 1 && depth_width % 2 == 0 && depth_height % 2 == 0))
                            scale_down_inplace_z16_by<2,2>(depth_image, depth_width /= 2, depth_height /= 2, depth_stride);
                        else if (qres == 2 && depth_width % 4 == 0 && depth_height % 4 == 0)
                            scale_down_inplace_z16_by<4,4>(depth_image, depth_width /= 4, depth_height /= 4, depth_stride);
                        rc_receiveImage(tracker, 0, rc_FORMAT_DEPTH16, ip->header.time, 0,
                                        depth_width, depth_height, depth_stride, depth_image,
                                        [](void *packet) { if (!--((packet_header_t *)packet)->sensor_id) free(packet); }, packet);
                    }

                    uint8_t *image = ip->data;
                    int width = ip->width, height = ip->height, stride = ip->width;
                    if ((qvga && width == 640 && height == 480) ||
                        (qres == 1 && width % 2 == 0 && height % 2 == 0))
                        scale_down_inplace_y8_by<2,2>(image, width /= 2, height /= 2, stride);
                    else if (qres == 2 && width % 4 == 0 && height % 4 == 0)
                        scale_down_inplace_y8_by<4,4>(image, width /= 4, height /= 4, stride);

                    if(last_image == sensor_clock::time_point()) last_image = data_timestamp;
                    last_image += image_interval;

                    rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
                                    width, height, stride, image,
                                    [](void *packet) { if (!--((packet_header_t *)packet)->sensor_id) free(packet); }, phandle.release());

                    is_stepping = false;
                    break;
                }
                case packet_image_raw:
                {
                    packet_image_raw_t *ip = (packet_image_raw_t *)packet;

                    if(image_decimate && data_timestamp < last_image) break;

                    if (ip->format == rc_FORMAT_GRAY8) {
                        if ((qvga && ip->width == 640 && ip->height == 480) ||
                            (qres == 1 && ip->width % 2 == 0 && ip->height % 2 == 0))
                            scale_down_inplace_y8_by<2,2>(ip->data, ip->width /= 2, ip->height /= 2, ip->stride);
                        else if (qres == 2 && ip->width % 4 == 0 && ip->height % 4 == 0)
                            scale_down_inplace_y8_by<4,4>(ip->data, ip->width /= 4, ip->height /= 4, ip->stride);
                        rc_receiveImage(tracker, packet->header.sensor_id, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
                                        ip->width, ip->height, ip->stride, ip->data,
                                        [](void *packet) { free(packet); }, phandle.release());
                    }
                    else if (use_depth && ip->format == rc_FORMAT_DEPTH16) {
                        if ((qvga && ip->width == 640 && ip->height == 480) ||
                            (qres == 1 && ip->width % 2 == 0 && ip->height % 2 == 0))
                            scale_down_inplace_z16_by<2,2>((uint16_t*)ip->data, ip->width /= 2, ip->height /= 2, ip->stride);
                        else if (qres == 2 && ip->width % 4 == 0 && ip->height % 4 == 0)
                            scale_down_inplace_z16_by<4,4>((uint16_t*)ip->data, ip->width /= 4, ip->height /= 4, ip->stride);
                        rc_receiveImage(tracker, packet->header.sensor_id, rc_FORMAT_DEPTH16, ip->header.time, ip->exposure_time_us,
                                        ip->width, ip->height, ip->stride, ip->data,
                                        [](void *packet) { free(packet); }, phandle.release());
                    }


                    last_image += image_interval;
                    is_stepping = false;
                    break;

                }
                case packet_stereo_raw:
                {

                    if (!is_stereo) {
                        is_stereo = true;
                        rc_configureStereo(tracker, 0, 1);
                    }
                    auto *ip = (packet_stereo_raw_t *)packet;

                    if (ip->format == rc_FORMAT_GRAY8) {
                        auto *data2 = ip->data + ip->stride1*ip->height;
                        if ((qvga && ip->width == 640 && ip->height == 480) ||
                            (qres == 1 && ip->width % 2 == 0 && ip->height % 2 == 0)) {
                            scale_down_inplace_y8_by<2,2>(ip->data, ip->width /= 2, ip->height /= 2, ip->stride1);
                            scale_down_inplace_y8_by<2,2>(    data2,ip->width     , ip->height     , ip->stride2);
                        } else if (qres == 2 && ip->width % 4 == 0 && ip->height % 4 == 0) {
                            scale_down_inplace_y8_by<4,4>(ip->data, ip->width /= 4, ip->height /= 4, ip->stride1);
                            scale_down_inplace_y8_by<4,4>(    data2,ip->width     , ip->height     , ip->stride2);
                        }
                        rc_receiveStereo(tracker, packet->header.sensor_id, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
                                         ip->width, ip->height, ip->stride1, ip->stride2, ip->data, (uint8_t *)data2,
                                         [](void *packet) { free(packet); }, phandle.release());
                    }

                    last_image += image_interval;
                    is_stepping = false;
                    break;
                }
                case packet_accelerometer:
                {
                    auto *p = (packet_accelerometer_t *)packet;
                    const rc_Vector acceleration_m__s2 = {{ p->a[0], p->a[1], p->a[2] }};

                    if(accel_decimate && data_timestamp < last_accel) break;
                    if(last_accel == sensor_clock::time_point()) last_accel = sensor_clock::micros_to_tp(header.time);
                    last_accel += accel_interval;

                    rc_receiveAccelerometer(tracker, packet->header.sensor_id, packet->header.time, acceleration_m__s2);
                    break;
                }
                case packet_gyroscope:
                {
                    auto *p = (packet_gyroscope_t *)packet;
                    const rc_Vector angular_velocity_rad__s = {{ p->w[0], p->w[1], p->w[2] }};

                    if(gyro_decimate && data_timestamp < last_gyro) break;
                    if(last_gyro == sensor_clock::time_point()) last_gyro = sensor_clock::micros_to_tp(header.time);
                    last_gyro += gyro_interval;

                    rc_receiveGyro(tracker, packet->header.sensor_id, packet->header.time, angular_velocity_rad__s);
                    break;
                }
                case packet_imu:
                {
                    auto imu = (packet_imu_t *)packet;
                    const rc_Vector acceleration_m__s2 = {{ imu->a[0], imu->a[1], imu->a[2] }}, angular_velocity_rad__s = {{ imu->w[0], imu->w[1], imu->w[2] }};
                    rc_receiveAccelerometer(tracker, packet->header.sensor_id, packet->header.time, acceleration_m__s2);
                    rc_receiveGyro(tracker, packet->header.sensor_id, packet->header.time, angular_velocity_rad__s);

                    break;
                }
                case packet_thermometer:
                {
                    auto thermometer = (packet_thermometer_t *)packet;
                    rc_receiveTemperature(tracker, packet->header.sensor_id, packet->header.time, thermometer->temperature_C);
                    break;
                }
                case packet_velocimeter:
                {
                    if(!use_odometry)
                        break;
                    auto velocimeter = (packet_velocimeter_t *)packet;
                    rc_receiveVelocimeter(tracker, velocimeter->header.sensor_id, velocimeter->header.time, rc_Vector{{velocimeter->v[0],velocimeter->v[1], velocimeter->v[2]}});
                    break;
                }
                case packet_diff_velocimeter:
                {
                    if(!use_odometry)
                        break;
                    auto diff_velocity = (packet_diff_velocimeter_t *)packet;
                    rc_receiveVelocimeter(tracker, diff_velocity->header.sensor_id,   diff_velocity->header.time, rc_Vector{{diff_velocity->v[0],0,0}});
                    rc_receiveVelocimeter(tracker, diff_velocity->header.sensor_id+1, diff_velocity->header.time, rc_Vector{{diff_velocity->v[1],0,0}});
                    break;
                }
                case packet_filter_control:
                {
                    // this legacy packet used a field in the header
                    // (which was renamed header.sensor_id) to control
                    // when measurement should be started
                    if(header.sensor_id == 1)
                    {
                        rc_setPose(tracker, rc_POSE_IDENTITY);
                        rc_startTracker(tracker, (async ? rc_RUN_ASYNCHRONOUS : rc_RUN_SYNCHRONOUS) | (fast_path ? rc_RUN_FAST_PATH : rc_RUN_NO_FAST_PATH));
                    }
                }
            }
            bytes_dispatched += header.bytes;
            packets_dispatched++;

            now = sensor_clock::now();
            // Update progress at most at 30Hz or if we are almost done
            if(progress_callback &&
               (now - last_progress > std::chrono::milliseconds(33) ||
                1.*bytes_dispatched / size > 0.99))
            {
                last_progress = now;
                progress_callback(bytes_dispatched / (float)size);
            }
        }
        
        file.read((char *)&header, 16);
        if(file.bad() || file.eof()) is_running = false;
    }
    rc_stopTracker(tracker);
    
    file.close();

    {
        auto fusion = (sensor_fusion *)tracker;
        v3 T = fusion->get_transformation().T;
        std::lock_guard<std::mutex> en_guard(lengths_mutex);
        length = (float) T.norm();
        path_length = fusion->sfm.s.total_distance;
    }
}

void replay::stop()
{
    is_running = false;
}

struct stream_position
{
    size_t position;
    string json;
};

size_t load_map_callback(void * handle, void *buffer, size_t length)
{
    struct stream_position * stream = (struct stream_position *)handle;

    const auto substr = stream->json.substr(stream->position, length);
    memcpy(buffer, substr.c_str(), substr.size());
    stream->position += substr.size();

    return substr.size();
}

bool replay::load_map(string filename)
{
    ifstream file_handle(filename, ios_base::binary);
    if(file_handle.fail())
        return false;

    struct stream_position s;
    s.json = std::string((istreambuf_iterator<char>(file_handle)), istreambuf_iterator<char>());
    s.position = 0;

    if(!rc_loadMap(tracker, load_map_callback, &s))
        return false;

    return true;
}

void save_map_callback(void *handle, const void *buffer, size_t length)
{
    std::ofstream * out = (std::ofstream *)handle;
    out->write((const char *)buffer, length);
}

void replay::save_map(string filename)
{
    std::ofstream out(filename, ios_base::binary);
    rc_saveMap(tracker, save_map_callback, &out);
}

void replay::set_stage() {
    rc_PoseTime pose = rc_getPose(tracker, nullptr, nullptr, rc_DATA_PATH_SLOW);
    rc_setStage(tracker, nullptr, pose.pose_m);
}
