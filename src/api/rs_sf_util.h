#pragma once
#ifndef rs_sf_util_h
#define rs_sf_util_h

#include "rs_shapefit.h"
#include <Eigen/Dense>
#include <list>
#include <vector>
#include <memory>
#if defined(OPENCV_FOUND) | defined(OpenCV_FOUND)
#include <opencv2/opencv.hpp>
#endif

static const int MAX_VALID_PID = 254;
static const int PLANE_SRC_PID = 255;

void set_to_zeros(rs_sf_image* img);
void eigen_3x3_real_symmetric(float D[6], float u[3], float v[3][3]);
void draw_planes(rs_sf_image* rgb, const rs_sf_image* map, const rs_sf_image* src = nullptr, const unsigned char* rgb_table[3] = nullptr, int num_color = 0);
void scale_plane_ids(rs_sf_image* map, int max_pid);

#endif // ! rs_sf_util_h

