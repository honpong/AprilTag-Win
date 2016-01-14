//  Copyright (c) 2015 RealityCap. All rights reserved.
#include "rc_replay.h"

#include <regex>
#include <sstream>
#include <memory>
#include <stdint.h>

using namespace rc;

enum packet_type {
    packet_camera = 1,
    packet_imu = 2,
    packet_accelerometer = 20,
    packet_gyroscope = 21,
    packet_filter_control = 25,
    packet_image_with_depth = 28,
};

typedef struct {
    uint32_t bytes; //size of packet including header
    uint16_t type;  //id of packet
    uint16_t user;  //packet-defined data
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

static bool read_file(const std::string name, std::basic_string<rc_char_t> &contents)
{
    std::basic_ifstream<rc_char_t> t(name);
    std::istreambuf_iterator<rc_char_t> b(t), e;
    contents.assign(b,e);
    return t.good() && b == e;
}

bool replay::set_calibration_from_filename(const std::string &fn)
{
    std::basic_string<rc_char_t> calibration;
    if(!read_file(fn + ".json", calibration)) {
        auto found = fn.find_last_of("/\\");
        std::string path = fn.substr(0, found+1);
        if(!read_file(path + "calibration.json", calibration))
            return false;
    }
    rcCalibration defaultCal = {};
    if (trace) if (trace) printf("rc_setCalibration(\"%s\", \" default={}\");\n", calibration.c_str());
    return rc_setCalibration(tracker, calibration.c_str(), &defaultCal);
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
    rc_setDataCallback(tracker, [](void *handle, rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count) {
        std::cout << time; for(int i=0; i<12; i++) std::cout << " " << pose[i]; std::cout << " " << feature_count << "\n";
    }, this);
}
void replay::enable_status_output()
{
    rc_setStatusCallback(tracker, [](void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress) {
        std::cerr << "state = " << state << " error = " << error << " confidence = " << confidence << " progress = " << progress << "\n";
    }, this);
}
void replay::enable_log_output(bool stream, rc_Timestamp period_us)
{
    rc_setLog(tracker, [](void *handle, const char *buffer_utf8, size_t length) {
        std::cerr << buffer_utf8 << "\n";
    }, stream, period_us, this);
}


bool replay::run()
{
    if (trace) printf("rc_startTracker(rc_E_SYNCRONOUS);\n");
    rc_startTracker(tracker, rc_E_SYNCRONOUS);

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
                rc_receiveImage(tracker, rc_EGRAY8, packet->header.time, 33333, nullptr/*pose estimate*/, false/*force_recognition*/,
                                width, height, width, packet->data + 16, [](void *packet) { free(packet); }, phandle.release());
            }   break;
            case packet_image_with_depth: {
                packet_image_with_depth_t *ip = (packet_image_with_depth_t *)packet;
                if(qvga) {
                    uint16_t *depth_image_src = (uint16_t*)(ip->data + ip->width * ip->height);
                    if (ip->width == 640 && ip->height == 480) {
                        int width = ip->width, height = ip->height;
                        uint8_t *image = ip->data;
                        ip->width = width / 2;
                        ip->height = height / 2;
                        for(int y = 0; y < ip->height; ++y) {
                            for(int x = 0; x < ip->width; ++x) {
                                image[y * ip->width + x] =
                                    (image[(y * 2 * width) + (x * 2)] +
                                     image[((y * 2 + 1) * width) + (x * 2)] +
                                     image[(y * 2 * width) + (x * 2 + 1)] +
                                     image[((y * 2 + 1) * width) + (x * 2 + 1)]) / 4;
                            }
                        }
                    }
                    uint16_t *depth_image_dst = (uint16_t*)(ip->data + ip->width * ip->height);
                    if(depth && ip->depth_width == 640 && ip->depth_height == 480) {
                        int width = ip->depth_width, height = ip->depth_height;
                        ip->depth_width = width / 2;
                        ip->depth_height = height / 2;
                        for(int y = 0; y < ip->depth_height; ++y) {
                            for(int x = 0; x < ip->depth_width; ++x) {
                                uint16_t p1 = depth_image_src[(y * 2 * width) + (x * 2)];
                                uint16_t p2 = depth_image_src[((y * 2 + 1) * width) + (x * 2)];
                                uint16_t p3 = depth_image_src[(y * 2 * width) + (x * 2 + 1)];
                                uint16_t p4 = depth_image_src[((y * 2 + 1) * width) + (x * 2 + 1)];
                                int divisor = !!p1 + !!p2 + !!p3 + !!p4;
                                depth_image_dst[y * ip->depth_width + x] = (p1 + p2 + p3 + p4) / (divisor ? divisor : 1);
                            }
                        }
                    } else {
                        for(int y = 0; y < ip->depth_height; ++y)
                            for(int x = 0; x < ip->depth_width; ++x)
                                depth_image_dst[y * ip->depth_width + x] = depth_image_src[y * ip->depth_width + x];
                    }
                }
                if(depth && ip->depth_height && ip->depth_width) {
                    ip->header.user = 2; // ref count
                    if (trace) printf("rc_receiveImageWithDepth(%lld, %lld, %dx%d, %dx%d);\n", packet->header.time, ip->exposure_time_us, ip->width, ip->height, ip->depth_width, ip->depth_height);
                    rc_receiveImageWithDepth(tracker, rc_EGRAY8, ip->header.time, ip->exposure_time_us, nullptr/*pose estimate*/, false /*force_recognition*/,
                                             ip->width, ip->height, ip->width, ip->data,
                                             [](void *packet) { if (!--((packet_header_t *)packet)->user) free(packet); }, packet,
                                             ip->depth_width, ip->depth_height, ip->depth_width*2, ip->data + ip->width * ip->height,
                                             [](void *packet) { if (!--((packet_header_t *)packet)->user) free(packet); }, phandle.release());
                } else {
                    if (trace) printf("rc_receiveImage(%lld, %lld, %dx%d);\n", packet->header.time, ip->exposure_time_us, ip->width, ip->height);
                    rc_receiveImage(tracker, rc_EGRAY8, ip->header.time, ip->exposure_time_us, nullptr/*pose estimate*/, false/*force_recognition*/,
                                    ip->width, ip->height, ip->width, ip->data, [](void *packet) { free(packet); }, phandle.release());
                }
            }   break;
            case packet_accelerometer: {
                const rc_Vector acceleration_m__s2 = { ((float *)packet->data)[0], ((float *)packet->data)[1], ((float *)packet->data)[2] };
                if (trace) printf("rc_receiveAccelerometer(%lld, %.9g, %.9g, %.9g);\n", packet->header.time, acceleration_m__s2.x, acceleration_m__s2.y, acceleration_m__s2.z);
                rc_receiveAccelerometer(tracker, packet->header.time, acceleration_m__s2);
            }   break;
            case packet_gyroscope: {
                const rc_Vector angular_velocity_rad__s = { ((float *)packet->data)[0], ((float *)packet->data)[1], ((float *)packet->data)[2] };
                if (trace) printf("rc_receiveGyro(%lld, %.9g, %.9g, %.9g);\n", packet->header.time, angular_velocity_rad__s.x, angular_velocity_rad__s.y, angular_velocity_rad__s.z);
                rc_receiveGyro(tracker, packet->header.time, angular_velocity_rad__s);
            }   break;
            case packet_imu: {
                auto imu = (packet_imu_t *)packet;
                const rc_Vector acceleration_m__s2 = { imu->a[0], imu->a[1], imu->a[2] }, angular_velocity_rad__s = { imu->w[0], imu->w[1], imu->w[2] };
                if (trace) printf("rc_receiveAccelerometer(%lld, %.9g, %.9g, %.9g);\n", packet->header.time, acceleration_m__s2.x, acceleration_m__s2.y, acceleration_m__s2.z);
                rc_receiveAccelerometer(tracker, packet->header.time, acceleration_m__s2);
                if (trace) printf("rc_receiveGyro(%lld, %.9g, %.9g, %.9g);\n", packet->header.time, angular_velocity_rad__s.x, angular_velocity_rad__s.y, angular_velocity_rad__s.z);
                rc_receiveGyro(tracker, packet->header.time, angular_velocity_rad__s);
            }   break;
            case packet_filter_control: {
                if(header.user == 1) { //start measuring
                    if (trace) printf("rc_setPose(rc_POSE_IDENTITY)\n");
                    rc_setPose(tracker, rc_POSE_IDENTITY);
                    if (trace) printf("rc_startTracker(rc_E_SYNCRONOUS)\n");
                    rc_startTracker(tracker, rc_E_SYNCRONOUS);
                }
            }   break;
        }
        rc_Pose endPose_m;
        rc_getPose(tracker, endPose_m);
        if (trace) printf("rc_getPose([3]=%.9f, [7]=%.9f, [11]=%.9f);\n", endPose_m[3], endPose_m[7], endPose_m[11]);
    }

    rc_Pose endPose_m;
    rc_getPose(tracker, endPose_m);
    if (trace) printf("rc_getPose([3]=%.9f, [7]=%.9f, [11]=%.9f);\n", endPose_m[3], endPose_m[7], endPose_m[11]);
    length_m = sqrtf(endPose_m[3]*endPose_m[3] + endPose_m[7]*endPose_m[7] + endPose_m[11]*endPose_m[11]);

    if (trace) printf("rc_stopTracker();\n");
    rc_stopTracker(tracker);
    file.close();

    return file.eof() && !file.fail();
}
