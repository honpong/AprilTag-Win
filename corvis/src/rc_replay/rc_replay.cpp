//  Copyright (c) 2015 RealityCap. All rights reserved.
#include "rc_replay.h"
#include <cinttypes>
#include <regex>
#include <sstream>
#include <memory>
#include <cstring>
#include <cstdint>
#include <set>

using namespace rc;

enum packet_type {
    packet_camera = 1,
    packet_imu = 2,
    packet_accelerometer = 20,
    packet_gyroscope = 21,
    packet_filter_control = 25,
    packet_image_with_depth = 28,
    packet_image_raw = 29,
    packet_thermometer = 31,
    packet_stereo_raw = 40,
};

typedef struct {
    uint32_t bytes; //size of packet including header
    uint16_t type;  //id of packet
    uint16_t sensor_id;  //id of sensor
    uint64_t time;  //time in microseconds
} packet_header_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} packet_camera_t;

typedef struct {
    packet_header_t header;
    uint64_t exposure_time_us;
    uint16_t width, height;
    uint16_t depth_width, depth_height;
    uint8_t data[];
} packet_image_with_depth_t;

typedef struct {
    packet_header_t header;
    uint64_t exposure_time_us;
    uint16_t width, height, stride;
    uint16_t format;
    uint8_t data[];
} packet_image_raw_t;

typedef struct {
    packet_header_t header;
    uint64_t exposure_time_us;
    uint16_t width, height, stride1, stride2;
    uint16_t format; // enum { Y8, Z16_mm };
    uint8_t data[]; // image2 starts at data + height*stride1
} packet_stereo_raw_t;

typedef struct {
    packet_header_t header;
    float a[3]; // m/s^2
    float w[3]; // rad/s
} packet_imu_t;

typedef struct {
    packet_header_t header;
    float a[3]; // m/s^2
} packet_accelerometer_t;

typedef struct {
    packet_header_t header;
    float w[3]; // rad/s
} packet_gyroscope_t;

typedef struct {
    packet_header_t header;
    float temperature_C;
} packet_thermometer_t;

bool replay::open(const char *name)
{
    file.open(name, std::ios::binary);
    if(file.fail()) {
        std::cerr << "Couldn't open file " << name << " for reading.\n";
        return false;
    }
    if (!set_calibration_from_filename(name)) {
        std::cerr << "Couldn't find calibration file for " << name << "\n";
        return false;
    }
    find_reference_in_filename(name);
    return true;
}

static bool read_file(const std::string name, std::string &contents)
{
    std::ifstream t(name);
    std::istreambuf_iterator<char> b(t), e;
    contents.assign(b,e);
    return t.good() && b == e;
}

bool replay::set_calibration_from_filename(const std::string &fn)
{
    std::string calibration;
    if(!read_file(fn + ".json", calibration)) {
        auto found = fn.find_last_of("/\\");
        std::string path = fn.substr(0, found+1);
        if(!read_file(path + "calibration.json", calibration))
            return false;
    }
    return rc_setCalibration(tracker, calibration.c_str());
}

static bool find_prefixed_number(const std::string in, const std::string &prefix, double &n)
{
    std::smatch m;
    if (!regex_search(in, m, std::regex(prefix + "(\\d+(?:\\.\\d*)?)"))) return false;
    std::stringstream s(m[1]);
    double nn; s >> nn; if (!s.fail()) n = nn;
    return !s.fail();
}

bool replay::find_reference_in_filename(const std::string &filename)
{
    double cm; bool found = find_prefixed_number(filename, "_L", cm);
    if (found) reference_length_m = cm / 100;
    return found;
}

void replay::enable_pose_output()
{
    rc_setDataCallback(tracker, [](void *handle, rc_Tracker * tracker, const rc_Data * data) {
        if (data->path != rc_DATA_PATH_SLOW) return;
        rc_PoseTime pt = rc_getPose(tracker, nullptr, nullptr, data->path);
        std::cout << pt.time_us; for(int c=0; c<4; c++) std::cout << " " << pt.pose_m.Q.v[c]; for(int c=0; c<3; c++) std::cout << " " << pt.pose_m.T.v[c]; std::cout << "\n";
    }, this);
}

void replay::enable_tum_output()
{
    rc_setDataCallback(tracker, [](void *handle, rc_Tracker * tracker, const rc_Data * data) {
        if (data->path != rc_DATA_PATH_SLOW) return;
        rc_PoseTime pt = rc_getPose(tracker, nullptr, nullptr, data->path);
        printf("%.9f", pt.time_us/1.e6);
        for(int c=0; c<3; c++) std::cout << " " << pt.pose_m.T.v[c] << " ";
        std::cout << pt.pose_m.Q.x << " " << pt.pose_m.Q.y << " " << pt.pose_m.Q.z << " " << pt.pose_m.Q.w << "\n";
    }, this);
}

void replay::enable_status_output()
{
    rc_setStatusCallback(tracker, [](void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence) {
        std::cerr << "state = " << state << " error = " << error << " confidence = " << confidence << "\n";
    }, this);
}

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

bool replay::run()
{
    bool is_stereo = false;
    typedef std::pair<std::string, int> data_pair;
    std::set<data_pair> unconfigured_data;

    rc_startTracker(tracker, rc_RUN_SYNCHRONOUS);

    while (file.peek() != EOF) {
        packet_header_t header;
        if (!file.read((char *)&header, 16))
            break;

        auto phandle = std::unique_ptr<void, void(*)(void *)>(malloc(header.bytes), free);
        auto packet = (packet_t *)phandle.get();
        packet->header = header;
        if (!file.read((char *)packet->data, header.bytes - 16))
            break;

        if (!sleep_until(header.time))
            break;

        switch(header.type) {
            case packet_camera: {
                char tmp[17]; memcpy(tmp, packet->data, 16); tmp[16] = 0;
                std::stringstream parse(tmp);
                int width, height; parse.ignore(3, ' ') >> width >> height; //pgm header is "P5 x y"
                int stride = width;
                if (qvga && width == 640 && height == 480)
                    scale_down_inplace_y8_by<2,2>(packet->data + 16, width /= 2, height /= 2, stride);
                if(!rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, packet->header.time, 33333,
                                    width, height, stride, packet->data + 16, [](void *packet) { free(packet); }, phandle.release()))
                    unconfigured_data.insert(data_pair("camera", 0));
            }   break;
            case packet_image_raw: {
                uint16_t sensor_id = packet->header.sensor_id;
                packet_image_raw_t *ip = (packet_image_raw_t *)packet;
                if (ip->format == rc_FORMAT_GRAY8) {
                    if (qvga && ip->width == 640 && ip->height == 480)
                        scale_down_inplace_y8_by<2,2>(ip->data, ip->width /= 2, ip->height /= 2, ip->stride);
                    if(!rc_receiveImage(tracker, packet->header.sensor_id, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
                                    ip->width, ip->height, ip->stride, ip->data,
                                    [](void *packet) { free(packet); }, phandle.release()))
                        unconfigured_data.insert(data_pair("camera", sensor_id));
                } else if (depth && ip->format == rc_FORMAT_DEPTH16) {
                    if (qvga && ip->width == 640 && ip->height == 480)
                        scale_down_inplace_z16_by<2,2>((uint16_t*)ip->data, ip->width /= 2, ip->height /= 2, ip->stride);
                    if(!rc_receiveImage(tracker, packet->header.sensor_id, rc_FORMAT_DEPTH16, ip->header.time, ip->exposure_time_us,
                                    ip->width, ip->height, ip->stride, ip->data,
                                    [](void *packet) { free(packet); }, phandle.release()))
                        unconfigured_data.insert(data_pair("depth", sensor_id));
                }
            }   break;
            case packet_stereo_raw: {
            if (!is_stereo) {
                is_stereo = true;
                rc_configureStereo(tracker, 0, 1);
            }
                packet_stereo_raw_t *ip = (packet_stereo_raw_t *)packet;

                if (ip->format == rc_FORMAT_GRAY8) {
                    rc_receiveStereo(tracker, packet->header.sensor_id, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
                                     ip->width, ip->height, ip->stride1, ip->stride2, ip->data, (uint8_t *)ip->data + ip->stride1*ip->height,
                                     [](void *packet) { free(packet); }, phandle.release());
                }
            }   break;
            case packet_image_with_depth: {
                packet_image_with_depth_t *ip = (packet_image_with_depth_t *)packet;
                ip->header.sensor_id = 1; // ref count
                if (depth && ip->depth_height && ip->depth_width) {
                    ip->header.sensor_id++; // ref count
                    uint16_t *depth_image = (uint16_t*)(ip->data + ip->width * ip->height);
                    int depth_width = ip->depth_width, depth_height = ip->depth_height, depth_stride = sizeof(uint16_t) * ip->depth_width;
                    if (qvga && depth_width == 640 && depth_height == 480)
                        scale_down_inplace_z16_by<2,2>(depth_image, depth_width /= 2, depth_height /= 2, depth_stride);
                    if(!rc_receiveImage(tracker, 0, rc_FORMAT_DEPTH16, ip->header.time, 0,
                                    depth_width, depth_height, depth_stride, depth_image,
                                    [](void *packet) { if (!--((packet_header_t *)packet)->sensor_id) free(packet); }, packet))
                        unconfigured_data.insert(data_pair("depth", 0));
                }
                {
                    uint8_t *image = ip->data;
                    int width = ip->width, height = ip->height, stride = ip->width;
                    if (qvga && width == 640 && height == 480)
                        scale_down_inplace_y8_by<2,2>(image, width /= 2, height /= 2, stride);
                    if(!rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, ip->header.time, ip->exposure_time_us,
                                    width, height, stride, image,
                                    [](void *packet) { if (!--((packet_header_t *)packet)->sensor_id) free(packet); }, phandle.release()))
                        unconfigured_data.insert(data_pair("camera", 0));
                }
            }   break;
            case packet_accelerometer: {
                const rc_Vector acceleration_m__s2 = { ((float *)packet->data)[0], ((float *)packet->data)[1], ((float *)packet->data)[2] };
                if(!rc_receiveAccelerometer(tracker, packet->header.sensor_id, packet->header.time, acceleration_m__s2))
                    unconfigured_data.insert(data_pair("accelerometer", packet->header.sensor_id));
            }   break;
            case packet_gyroscope: {
                const rc_Vector angular_velocity_rad__s = { ((float *)packet->data)[0], ((float *)packet->data)[1], ((float *)packet->data)[2] };
                if(!rc_receiveGyro(tracker, packet->header.sensor_id, packet->header.time, angular_velocity_rad__s))
                    unconfigured_data.insert(data_pair("gyroscope", packet->header.sensor_id));
            }   break;
            case packet_imu: {
                auto imu = (packet_imu_t *)packet;
                const rc_Vector acceleration_m__s2 = { imu->a[0], imu->a[1], imu->a[2] }, angular_velocity_rad__s = { imu->w[0], imu->w[1], imu->w[2] };
                if(!rc_receiveAccelerometer(tracker, packet->header.sensor_id, packet->header.time, acceleration_m__s2))
                    unconfigured_data.insert(data_pair("accelerometer", packet->header.sensor_id));
                if(!rc_receiveGyro(tracker, packet->header.sensor_id, packet->header.time, angular_velocity_rad__s))
                    unconfigured_data.insert(data_pair("gyroscope", packet->header.sensor_id));
            }   break;
            case packet_thermometer: {
                auto thermometer = (packet_thermometer_t *)packet;
                if(!rc_receiveTemperature(tracker, thermometer->header.sensor_id, thermometer->header.time, thermometer->temperature_C))
                    unconfigured_data.insert(data_pair("temperature", packet->header.sensor_id));
            }   break;
            case packet_filter_control: {
                if(header.sensor_id == 1) { //start measuring
                    rc_setPose(tracker, rc_POSE_IDENTITY);
                    rc_startTracker(tracker, rc_RUN_SYNCHRONOUS);
                }
            }   break;
        }
        rc_PoseTime endPoseTime = rc_getPose(tracker,nullptr,nullptr,rc_DATA_PATH_SLOW);
    }

    rc_PoseTime endPoseTime = rc_getPose(tracker,nullptr,nullptr,rc_DATA_PATH_SLOW);
    length_m = sqrtf(endPoseTime.pose_m.T.x*endPoseTime.pose_m.T.x + endPoseTime.pose_m.T.y*endPoseTime.pose_m.T.y + endPoseTime.pose_m.T.z*endPoseTime.pose_m.T.z);

    rc_stopTracker(tracker);
    file.close();
    if(unconfigured_data.size())
        for(auto & data : unconfigured_data)
            std::cerr << "Warning: Received data for " << data.first << " id " << data.second << " before it was configured\n";

    return file.eof() && !file.fail();
}

void replay::start_mapping(bool relocalize) {
    rc_startMapping(tracker,relocalize);
}
