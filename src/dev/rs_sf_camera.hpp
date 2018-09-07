#pragma once

#ifndef rs_sf_camera_hpp
#define rs_sf_camera_hpp

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

struct rs_sf_image_stream
{
    virtual rs_sf_image* get_images() = 0;
    virtual rs_sf_intrinsics* get_intrinsics(int stream = RS_SF_STREAM_DEPTH) = 0;
    virtual rs_sf_extrinsics* get_extrinsics(int from_stream = RS_SF_STREAM_COLOR, int to_stream = RS_SF_STREAM_DEPTH) = 0;
    virtual float get_depth_unit() = 0;
    virtual ~rs_sf_image_stream() {}
};

std::unique_ptr<rs_sf_image_stream> rs_sf_create_camera_stream(int w, int h);

#endif //!rs_sf_camera_hpp
