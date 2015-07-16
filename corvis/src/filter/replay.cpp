//
//  replay.cpp
//
//  Created by Eagle Jones on 4/8/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "replay.h"

#include <string.h>
#include <regex>
#include "calibration_json_store.h"
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

bool load_calibration(string filename, corvis_device_parameters & dc)
{
    ifstream file_handle(filename);
    if(file_handle.fail())
        return false;

    string json((istreambuf_iterator<char>(file_handle)), istreambuf_iterator<char>());

    if(!calibration_deserialize(json, dc))
        return false;

    return true;
}

bool replay::set_calibration_from_filename(const char *filename)
{
    corvis_device_parameters dc;
    string fn(filename);
    if(!load_calibration(fn + ".json", dc)) {
        auto found = fn.find_last_of("/\\");
        string path = fn.substr(0, found+1);
        if(!load_calibration(path + "calibration.json", dc))
            return false;
    }
    fusion.set_device(dc);
    return true;
}

bool replay::set_reference_from_filename(const string &filename)
{
    return load_reference_from_pose_file(filename + ".pose")
        || find_reference_in_filename(filename);
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
    stringstream s(m[1]);
    double nn; s >> nn; if (!s.fail()) n = nn;
    return !s.fail();
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
        fusion.camera_callback = [this](std::unique_ptr<sensor_fusion::data> data, camera_data &&image)
        {
            camera_callback(&fusion.sfm, std::move(image));
        };
    }
    fusion.start_offline();
}

image_gray8 replay::parse_gray8(int width, int height, int stride, uint8_t *data, uint64_t time_us, uint64_t exposure_time_us, std::unique_ptr<void, void(*)(void *)> handle)
{
    image_gray8 gray;
    gray.image = data;
    gray.width = width;
    gray.height = height;
    gray.stride = width;
    if(qvga && width == 640 && height == 480)
    {
        gray.width = width / 2;
        gray.height = height / 2;
        gray.stride = width / 2;
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

void replay::start()
{
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
                    camera_data d = parse_gray8(width, height, width, packet->data + 16, packet->header.time, 33333, std::move(phandle));
                    fusion.receive_image(std::move(d));
                    is_stepping = false;
                    break;
                }
                case packet_image_with_depth:
                {
                    packet_image_with_depth_t *ip = (packet_image_with_depth_t *)packet;
                    camera_data d = parse_gray8(ip->width, ip->height, ip->width, ip->data, ip->header.time, ip->exposure_time_us, std::move(phandle));
                    if(depth && ip->depth_height && ip->depth_width)
                    {
                        d.depth = std::make_unique<image_depth16>();
                        d.depth->width = ip->depth_width;
                        d.depth->height = ip->depth_height;
                        d.depth->stride = ip->depth_width * 2;
                        d.depth->timestamp = d.timestamp;
                        d.depth->exposure_time = d.exposure_time;
                        d.depth->image = (uint16_t *)(ip->data + ip->width * ip->height);
                        int width = d.depth->width;
                        int height = d.depth->height;
                        int stride = d.depth->stride;
                        if(qvga && width == 640 && height == 480)
                        {
                            d.depth->width = width / 2;
                            d.depth->height = height / 2;
                            d.depth->stride = stride / 2;
                            for(int y = 0; y < d.depth->height; ++y) {
                                for(int x = 0; x < d.depth->stride; ++x) {
                                    uint16_t p1 = d.depth->image[(y * 2 * width) + (x * 2)];
                                    uint16_t p2 = d.depth->image[((y * 2 + 1) * width) + (x * 2)];
                                    uint16_t p3 = d.depth->image[(y * 2 * width) + (x * 2 + 1)];
                                    uint16_t p4 = d.depth->image[((y * 2 + 1) * width) + (x * 2 + 1)];
                                    int divisor = 0;
                                    if(p1) divisor++;
                                    if(p2) divisor++;
                                    if(p3) divisor++;
                                    if(p4) divisor++;
                                    if(!divisor) divisor = 1;
                                    d.depth->image[y * d.depth->stride / 2 + x] = (p1 + p2 + p3 + p4) / divisor;
                                }
                            }
                        }
                    }
                    fusion.receive_image(std::move(d));
                    is_stepping = false;
                    break;
                }
                case packet_accelerometer:
                {
                    accelerometer_data d;
                    d.accel_m__s2[0] = ((float *)packet->data)[0];
                    d.accel_m__s2[1] = ((float *)packet->data)[1];
                    d.accel_m__s2[2] = ((float *)packet->data)[2];
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    fusion.receive_accelerometer(std::move(d));
                    break;
                }
                case packet_gyroscope:
                {
                    gyro_data d;
                    d.angvel_rad__s[0] = ((float *)packet->data)[0];
                    d.angvel_rad__s[1] = ((float *)packet->data)[1];
                    d.angvel_rad__s[2] = ((float *)packet->data)[2];
                    d.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    fusion.receive_gyro(std::move(d));
                    break;
                }
                case packet_imu:
                {
                    accelerometer_data a;
                    auto imu = (packet_imu_t *)packet;
                    a.accel_m__s2[0] = imu->a[0];
                    a.accel_m__s2[1] = imu->a[1];
                    a.accel_m__s2[2] = imu->a[2];
                    a.timestamp = sensor_clock::time_point(std::chrono::microseconds(header.time));
                    fusion.receive_accelerometer(std::move(a));
                    gyro_data g;
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

    v4 T = fusion.get_transformation().T;
    length = (float) T.norm();
    path_length = fusion.sfm.s.total_distance;
}

bool replay::configure_all(const char *filename, bool realtime, std::function<void (float)> progress, std::function<void (const filter *, camera_data)> camera_cb)
{
    if(!open(filename)) return false;
    if (!set_calibration_from_filename(filename))
        return false;
    is_realtime = realtime;
    progress_callback = progress;
    camera_callback = camera_cb;
    setup_filter();
    return true;
}

void replay::stop()
{
    is_running = false;
}
