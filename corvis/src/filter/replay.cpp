//
//  replay.cpp
//
//  Created by Eagle Jones on 4/8/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "replay.h"

#include <string.h>
#include <regex>
#include "calibration_json.h"
#include "device_parameters.h"
#include "../cor/packet.h"

bool replay::open(const char *name)
{
    file.open(name, ios::binary);
    if(file.bad())
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
    auto device = fusion.get_device();
    for(int i = 0; i < 3; i++) {
        device.imu.a_bias_m__s2[i] = 0;
        device.imu.w_bias_rad__s[i] = 0;
        device.imu.a_bias_var_m2__s4[i] = 1e-3;
        device.imu.w_bias_var_rad2__s2[i] = 1e-4;
    }
    fusion.set_device(device);

}

bool replay::load_calibration(std::string filename)
{
    ifstream file_handle(filename);
    if(file_handle.fail())
        return false;

    string json((istreambuf_iterator<char>(file_handle)), istreambuf_iterator<char>());

    device_parameters dc;
    if(!calibration_deserialize(json, dc))
        return false;

    calibration_file = filename;
    fusion.set_device(dc);
    return true;
}

bool replay::set_calibration_from_filename(const char *filename)
{
    string fn(filename), json;
    if(!load_calibration(json = fn + ".json")) {
        auto found = fn.find_last_of("/\\");
        string path = fn.substr(0, found+1);
        if(!load_calibration(json = path + "calibration.json"))
            return false;
    }
    return true;
}

bool replay::get_reference_pose(const sensor_clock::time_point & timestamp, tpose & pose_out)
{
    return reference_seq && reference_seq->get_pose(timestamp, pose_out);
}

bool replay::set_reference_from_filename(const string &filename)
{
    return load_reference_from_pose_file(filename + ".pose") ||
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
    if(camera_callback)
    {
        fusion.camera_callback = [this](std::unique_ptr<sensor_fusion::data> data, image_gray8 &&image)
        {
            camera_callback(&fusion.sfm, std::move(image));
        };
    }
    fusion.start_offline();
}

image_gray8 replay::parse_gray8(int width, int height, int stride, uint8_t *data, uint64_t time_us, uint64_t exposure_time_us, std::unique_ptr<void, void(*)(void *)> handle)
{
    image_gray8 gray;
    gray.source = &fusion.camera;
    gray.image = data;
    gray.width = width;
    gray.height = height;
    gray.stride = stride;
    if(qvga && width == 640 && height == 480)
    {
        gray.width = width / 2;
        gray.height = height / 2;
        gray.stride = stride / 2;
        for(int y = 0; y < gray.height; ++y) {
            for(int x = 0; x < gray.width; ++x) {
                gray.image[y * gray.stride + x] =
                (gray.image[(y * 2 * width) + (x * 2)] +
                 gray.image[((y * 2 + 1) * width) + (x * 2)] +
                 gray.image[(y * 2 * width) + (x * 2 + 1)] +
                 gray.image[((y * 2 + 1) * width) + (x * 2 + 1)]) / 4;
            }
        }
    }
    gray.exposure_time = std::chrono::microseconds(exposure_time_us);
    gray.timestamp = sensor_clock::time_point(std::chrono::microseconds(time_us));
    gray.image_handle = std::move(handle);
    return gray;
}

image_depth16 replay::parse_depth16(int width, int height, int stride, uint16_t *data, uint64_t time_us, uint64_t exposure_time_us, std::unique_ptr<void, void(*)(void *)> handle)
{
    image_depth16 depth;
    depth.source = &fusion.depth;
    depth.width = width;
    depth.height = height;
    depth.stride = stride;
    depth.timestamp = sensor_clock::time_point(std::chrono::microseconds(time_us));
    depth.exposure_time = std::chrono::microseconds(exposure_time_us);
    depth.image = data;
    if(qvga && width == 640 && height == 480)
    {
        depth.width = width / 2;
        depth.height = height / 2;
        depth.stride = width / 2 * 2;
        for(int y = 0; y < depth.height; ++y) {
            for(int x = 0; x < depth.width; ++x) {
                uint16_t p1 = depth.image[(y * 2 * width) + (x * 2)];
                uint16_t p2 = depth.image[((y * 2 + 1) * width) + (x * 2)];
                uint16_t p3 = depth.image[(y * 2 * width) + (x * 2 + 1)];
                uint16_t p4 = depth.image[((y * 2 + 1) * width) + (x * 2 + 1)];
                int divisor = !!p1 + !!p2 + !!p3 + !!p4;
                depth.image[depth.width * y + x] = (p1 + p2 + p3 + p4) / (divisor ? divisor : 1);
            }
        }
    }
    depth.image_handle = std::move(handle);
    return depth;
}

void replay::start(string map_filename)
{
    setup_filter();
    if(!map_filename.empty())
        load_map(map_filename);
    is_running = true;
    path_length = 0;
    length = 0;
    packets_dispatched = 0;
    bytes_dispatched = 0;

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
            auto timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time)) + realtime_offset;
            now = sensor_clock::now();
            if(is_realtime && timestamp - now > std::chrono::seconds(1)) {
                auto gap = std::chrono::duration_cast<std::chrono::microseconds>(timestamp - now);
                fprintf(stderr, "Warning: skipping a %f second gap\n", gap.count()/1.e6f);
                realtime_offset -= (timestamp - now);
                timestamp -= realtime_offset;
            }
            if(is_realtime && timestamp - now > std::chrono::microseconds(0))
                std::this_thread::sleep_for(timestamp - now);

            switch(header.type)
            {
                case packet_camera:
                {
                    int width, height;
                    char tmp[17];
                    memcpy(tmp, packet->data, 16);
                    tmp[16] = 0;
                    std::stringstream parse(tmp);
                    //pgm header is "P5 x y"
                    parse.ignore(3, ' ') >> width >> height;
                    image_gray8 d = parse_gray8(width, height, width, packet->data + 16, packet->header.time, 33333, std::move(phandle));
                    if(image_decimate && d.timestamp < last_image) break;
                    if(last_image == sensor_clock::time_point()) last_image = d.timestamp;
                    last_image += image_interval;

                    fusion.receive_image(std::move(d));
                    is_stepping = false;
                    break;
                }
                case packet_image_with_depth:
                {
                    packet_image_with_depth_t *ip = (packet_image_with_depth_t *)packet;
                    image_gray8 d = parse_gray8(ip->width, ip->height, ip->width, ip->data, ip->header.time, ip->exposure_time_us, std::move(phandle));
                    if(image_decimate && d.timestamp < last_image) break;
                    if(use_depth && ip->depth_height && ip->depth_width)
                    {
                        auto dhandle = std::unique_ptr<void, void(*)(void *)>(malloc(header.bytes), free);
                        auto depth_data = (uint16_t *)dhandle.get();
                        memcpy(depth_data, ip->data + ip->width * ip->height, sizeof(uint16_t)*ip->depth_width*ip->depth_height);
                        image_depth16 depth = parse_depth16(ip->depth_width, ip->depth_height, ip->depth_width*2, depth_data, ip->header.time, ip->exposure_time_us, std::move(dhandle));
                        fusion.receive_image(std::move(depth));
                    }
                    if(last_image == sensor_clock::time_point()) last_image = d.timestamp;
                    last_image += image_interval;
                    fusion.receive_image(std::move(d));
                    is_stepping = false;
                    break;
                }
                case packet_image_raw:
                {
                    packet_image_raw_t *ip = (packet_image_raw_t *)packet;
                    if(image_decimate &&
                       sensor_clock::micros_to_tp(ip->header.time) < last_image)
                            break;

                    if(ip->format == rc_FORMAT_GRAY8) {
                        image_gray8 d = parse_gray8(ip->width, ip->height, ip->width, ip->data, ip->header.time, ip->exposure_time_us, std::move(phandle));
                        fusion.receive_image(std::move(d));
                    }
                    else if(ip->format == rc_FORMAT_DEPTH16) {
                        if(!use_depth)
                            break;

                        image_depth16 d = parse_depth16(ip->width, ip->height, ip->stride, (uint16_t *)ip->data, ip->header.time, ip->exposure_time_us, std::move(phandle));
                        fusion.receive_image(std::move(d));
                    }
                    else {
                        fprintf(stderr, "Error: Unsupported packet_image_raw format\n");
                        break;
                    }

                    last_image += image_interval;
                    is_stepping = false;
                    break;

                }
                case packet_accelerometer:
                {
                    accelerometer_data d;
                    d.source = &fusion.accelerometer;
                    d.accel_m__s2[0] = ((float *)packet->data)[0];
                    d.accel_m__s2[1] = ((float *)packet->data)[1];
                    d.accel_m__s2[2] = ((float *)packet->data)[2];
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    if(accel_decimate && d.timestamp < last_accel) break;
                    if(last_accel == sensor_clock::time_point()) last_accel = d.timestamp;
                    last_accel += accel_interval;
                    fusion.receive_accelerometer(std::move(d));
                    break;
                }
                case packet_gyroscope:
                {
                    gyro_data d;
                    d.source = &fusion.gyro;
                    d.angvel_rad__s[0] = ((float *)packet->data)[0];
                    d.angvel_rad__s[1] = ((float *)packet->data)[1];
                    d.angvel_rad__s[2] = ((float *)packet->data)[2];
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    if(gyro_decimate && d.timestamp < last_gyro) break;
                    if(last_gyro == sensor_clock::time_point()) last_gyro = d.timestamp;
                    last_gyro += gyro_interval;
                    fusion.receive_gyro(std::move(d));
                    break;
                }
                case packet_imu:
                {
                    accelerometer_data a;
                    a.source = &fusion.accelerometer;
                    auto imu = (packet_imu_t *)packet;
                    a.accel_m__s2[0] = imu->a[0];
                    a.accel_m__s2[1] = imu->a[1];
                    a.accel_m__s2[2] = imu->a[2];
                    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    fusion.receive_accelerometer(std::move(a));
                    gyro_data g;
                    g.source = &fusion.gyro;
                    g.angvel_rad__s[0] = imu->w[0];
                    g.angvel_rad__s[1] = imu->w[1];
                    g.angvel_rad__s[2] = imu->w[2];
                    g.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    fusion.receive_gyro(std::move(g));
                    break;
                }
                case packet_filter_control:
                {
                    if(header.user == 1)
                    {
                        //start measuring
                        fusion.queue->dispatch_sync([this] { filter_set_reference(&fusion.sfm); });
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
    fusion.stop();
    
    file.close();

    {
        v3 T = fusion.get_transformation().T;
        std::lock_guard<std::mutex> en_guard(lengths_mutex);
        length = (float) T.norm();
        path_length = fusion.sfm.s.total_distance;
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
    size_t chars = stream->json.copy((char *)buffer, length, stream->position);
    stream->position += chars;
    return chars;
}

bool replay::load_map(string filename)
{
    ifstream file_handle(filename);
    if(file_handle.fail())
        return false;

    struct stream_position s;
    s.json = std::string((istreambuf_iterator<char>(file_handle)), istreambuf_iterator<char>());
    s.position = 0;

    if(!fusion.load_map(load_map_callback, &s))
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
    std::ofstream out(filename);
    fusion.save_map(save_map_callback, &out);
}
