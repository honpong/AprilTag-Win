#pragma once

#ifndef rs_sf_camera_hpp
#define rs_sf_camera_hpp

#include <map>
#include "librealsense2/rs.hpp"
#include "rs_shapefit.h"

const static auto RS_SF_STREAM_DEPTH = RS2_STREAM_DEPTH;
const static auto RS_SF_STREAM_COLOR = RS2_STREAM_COLOR;
const static auto RS_SF_STREAM_INFRARED = RS2_STREAM_INFRARED;
const static auto RS_SF_STREAM_COUNT = RS2_STREAM_COUNT;
const static auto RS_SF_FORMAT_Z16 = RS2_FORMAT_Z16;
const static auto RS_SF_FORMAT_Y8 = RS2_FORMAT_Y8;
const static auto RS_SF_FORMAT_RGB8 = RS2_FORMAT_RGB8;
const static auto RS_SF_FORMAT_BGR8 = RS2_FORMAT_BGR8;

struct rs_sf_stream_id
{
    int stream_type;
    int stream_index;
};

static bool operator<(const rs_sf_stream_id& a, const rs_sf_stream_id& b){
    return (a.stream_type * RS2_STREAM_COUNT + a.stream_index) < (b.stream_type * RS2_STREAM_COUNT + b.stream_index);
}

struct rs_sf_device_manager
{
protected:
    rs2::context          _ctx;
    rs2::pipeline         _pipeline;
    rs2::pipeline_profile _pprofile;
    rs2::config           _config;
    rs2::device           _device;

    struct stream_buffer {
        rs2_intrinsics                            _intrinsics;
        std::map<rs_sf_stream_id, rs2_extrinsics> _extrinsics;
        rs_sf_image                               _image[3];
        float                                     _depth_unit = 0.001f;
    };
    std::map<rs_sf_stream_id, stream_buffer> _stream_buffer;
    int _stream_to_byte_per_pixel[RS_SF_STREAM_COUNT] = { 0,2,3,1,1 };
    
    bool search_device_name(const std::string device_name) const;
    static void print(const rs2::error& e);
};

struct rs_sf_data_stream
{
    virtual rs_sf_data* get_data() = 0;
    virtual rs_sf_intrinsics* get_intrinsics(const rs_sf_stream_id& stream_id) = 0;
    virtual rs_sf_extrinsics* get_extrinsics(const rs_sf_stream_id& from_stream, const rs_sf_stream_id& to_stream) = 0;
    virtual float get_depth_unit() = 0;
    virtual ~rs_sf_data_stream() {}
};

struct rs_sf_image_stream
{
    virtual rs_sf_image* get_images() = 0;
    virtual rs_sf_intrinsics* get_intrinsics(int stream = RS_SF_STREAM_DEPTH) = 0;
    virtual rs_sf_extrinsics* get_extrinsics(int from_stream = RS_SF_STREAM_COLOR, int to_stream = RS_SF_STREAM_DEPTH) = 0;
    virtual float get_depth_unit() = 0;
    virtual ~rs_sf_image_stream() {}
};

std::unique_ptr<rs_sf_image_stream> rs_sf_create_camera_stream(int w, int h);
std::unique_ptr<rs_sf_data_stream> rs_sf_create_camera_imu_stream(int w, int h);

#endif //!rs_sf_camera_hpp
