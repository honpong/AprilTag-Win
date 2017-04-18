#pragma once

#ifndef rs_sf_camera_hpp
#define rs_sf_camera_hpp

#include <../../thirdparty/LibRealSense/librealsense/include/librealsense/rs.h>
#include "rs_shapefit.h"

const static auto RS_SF_STREAM_DEPTH = RS_STREAM_DEPTH;
const static auto RS_SF_STREAM_COLOR = RS_STREAM_COLOR;
const static auto RS_SF_STREAM_INFRARED = RS_STREAM_INFRARED;
const static auto RS_SF_STREAM_COUNT = RS_STREAM_COUNT;
const static auto RS_SF_FORMAT_Z16 = RS_FORMAT_Z16;
const static auto RS_SF_FORMAT_Y8 = RS_FORMAT_Y8;
const static auto RS_SF_FORMAT_RGB8 = RS_FORMAT_RGB8;
const static auto RS_SF_FORMAT_BGR8 = RS_FORMAT_BGR8;

struct rs_sf_image_stream
{
    virtual rs_sf_image* get_images() = 0;
    virtual rs_sf_intrinsics* get_intrinsics() = 0;
};

std::unique_ptr<rs_sf_image_stream> rs_sf_create_camera_stream(int w, int h);

#endif //!rs_sf_camera_hpp
