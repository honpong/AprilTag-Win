#pragma once

#ifndef rs_sf_camera_hpp
#define rs_sf_camera_hpp

#include "rs_shapefit.h"
#include <librealsense2/rs.hpp>

const static auto RS_SF_STREAM_DEPTH = RS2_STREAM_DEPTH;
const static auto RS_SF_STREAM_COLOR = RS2_STREAM_COLOR;
const static auto RS_SF_STREAM_INFRARED = RS2_STREAM_INFRARED;
const static auto RS_SF_STREAM_COUNT = RS2_STREAM_COUNT;
const static auto RS_SF_FORMAT_Z16 = RS2_FORMAT_Z16;
const static auto RS_SF_FORMAT_Y8 = RS2_FORMAT_Y8;
const static auto RS_SF_FORMAT_RGB8 = RS2_FORMAT_RGB8;
const static auto RS_SF_FORMAT_BGR8 = RS2_FORMAT_BGR8;
const static int  RS_SF_CALIBRATION_FILE_VERSION = 3;

#include <list>
struct rs_sf_data_buf : public rs_sf_data { virtual ~rs_sf_data_buf(){} };
typedef std::shared_ptr<rs_sf_data_buf> rs_sf_data_ptr;
struct rs_sf_data_list : public std::list<rs_sf_data_ptr>
{
    rs_sf_data_ptr& operator[](int i){ auto it = begin(); std::advance(it, i); return *it; }
};
typedef std::vector<rs_sf_data_list>   rs_sf_dataset;
typedef std::shared_ptr<rs_sf_dataset> rs_sf_dataset_ptr;

struct rs_sf_stream_info
{
    rs_sf_sensor_t                type;
    rs_sf_uint16_t                index;
    float                         fps;
    int                           format;
    union intrinsics_t {
        rs_sf_intrinsics          cam_intrinsics;
        rs_sf_imu_intrinsics      imu_intrinsics;
    } intrinsics;
    std::vector<rs_sf_extrinsics> extrinsics;
};
typedef rs_sf_stream_info::intrinsics_t rs_sf_stream_intrinsics;

struct rs_sf_data_stream
{
    typedef std::vector<rs_sf_stream_info> stream_info_vec;
    typedef std::vector<std::string> string_vec;
    virtual rs_sf_dataset_ptr wait_for_data(const std::chrono::milliseconds& wait_time_us = std::chrono::milliseconds(34)) = 0;
    virtual std::string       get_device_name() = 0;
    virtual string_vec        get_device_info() = 0;
    virtual stream_info_vec   get_stream_info() = 0;
    virtual float             get_depth_unit()  = 0;
    virtual ~rs_sf_data_stream() {}
};

struct rs_sf_data_writer
{
    virtual ~rs_sf_data_writer() {}
    virtual bool write(rs_sf_dataset& data) = 0;
};

struct rs_sf_image_stream
{
    virtual rs_sf_image*      get_images() = 0;
    virtual rs_sf_intrinsics* get_intrinsics(int stream = RS_SF_STREAM_DEPTH) = 0;
    virtual rs_sf_extrinsics* get_extrinsics(int from_stream = RS_SF_STREAM_COLOR, int to_stream = RS_SF_STREAM_DEPTH) = 0;
    virtual float get_depth_unit() = 0;
    virtual ~rs_sf_image_stream() {}
};

std::unique_ptr<rs_sf_image_stream> rs_sf_create_camera_stream(int w, int h);
std::unique_ptr<rs_sf_data_stream>  rs_sf_create_camera_imu_stream(int w, int h, int laser);
std::unique_ptr<rs_sf_data_stream>  rs_sf_create_camera_imu_stream(const std::string& folder_path);
std::unique_ptr<rs_sf_data_writer>  rs_sf_create_data_writer(rs_sf_data_stream* src, const std::string& path);

#endif //!rs_sf_camera_hpp
