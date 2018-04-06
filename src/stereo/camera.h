#ifndef __CAMERA_H
#define __CAMERA_H

#include "cor_types.h"
#include "vec4.h"

class camera {
public:
    int width, height, nchannels;
    f_t center_x, center_y;
    f_t k1, k2, k3;
    f_t focal_length;

    camera(): width(0), height(0), nchannels(1), center_x(0), center_y(0), k1(0), k2(0), k3(0), focal_length(0) {}
    camera(int _width, int _height, int _nchannels, f_t _center_x, f_t _center_y, f_t _k1, f_t _k2, f_t _k3, f_t _focal_length) : width(_width), height(_height), nchannels(_nchannels), center_x(_center_x), center_y(_center_y), k1(_k1), k2(_k2), k3(_k3), focal_length(_focal_length) {}

    v3 project_image_point(f_t x, f_t y) const;
    v3 calibrate_image_point(f_t x, f_t y) const;
    feature_t undistort_image_point(f_t x, f_t y) const;
    void undistort_image(const uint8_t * src, uint8_t * dest, bool * valid) const;
};

#endif

