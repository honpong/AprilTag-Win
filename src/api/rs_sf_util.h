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

typedef Eigen::Matrix3f m3;
typedef Eigen::Vector3f v3;
typedef Eigen::Vector2f v2;
typedef Eigen::Vector2i i2;
typedef Eigen::Matrix<unsigned char, 3, 1> b3;
typedef Eigen::Map<v3> v3_map;

struct rs_shapefit 
{
    virtual ~rs_shapefit() {};
    rs_sf_intrinsics m_intrinsics;
};

struct pose_t
{
    m3 rotation;
    v3 translation;

    inline pose_t& set_pose(const float p[12] = nullptr) {
        if (p) {
            rotation << p[0], p[1], p[2], p[4], p[5], p[6], p[8], p[9], p[10];
            translation << p[3], p[7], p[11];
        }
        else { rotation.setIdentity(); translation.setZero(); }
        return *this;
    }
    inline v3 transform(const v3& p) const { return rotation*p + translation; }
    inline pose_t invert() const { return pose_t{ rotation.transpose(),-(rotation.transpose() * translation) }; }
};

void set_to_zeros(rs_sf_image* img);
void eigen_3x3_real_symmetric(float D[6], float u[3], float v[3][3]);
void draw_planes(rs_sf_image* rgb, const rs_sf_image* map, const rs_sf_image* src = nullptr, const unsigned char* rgb_table[3] = nullptr, int num_color = 0);
void scale_plane_ids(rs_sf_image* map, int max_pid);
void draw_line_rgb(rs_sf_image * rgb, const v2& p0, const v2& p1, const b3& color, const int size = 2);
void draw_boxes(rs_sf_image* rgb, const rs_sf_intrinsics& camera, const std::vector<rs_sf_box>& boxes);

#endif // ! rs_sf_util_h

