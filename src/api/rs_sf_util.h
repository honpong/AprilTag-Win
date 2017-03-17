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
static const float FLOAT_MAX_VALUE = std::numeric_limits<float>::max();
static const float FLOAT_MIN_VALUE = -FLOAT_MAX_VALUE;

typedef Eigen::Matrix<float, 3, 3, Eigen::RowMajor> m3;
typedef Eigen::Vector3f v3;
typedef Eigen::Vector2f v2;
typedef Eigen::Vector2i i2;
typedef Eigen::Matrix<unsigned char, 3, 1> b3;
typedef Eigen::Map<v3> v3_map;
typedef Eigen::Map<Eigen::Matrix<float, 3, 3, Eigen::ColMajor>> m3_axis_map;
typedef std::vector<i2> contour;
typedef Eigen::Quaternionf qv3;

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


inline void print_box(const rs_sf_box& box)
{
    printf("a0 %.1f %.1f %.1f \n", box.axis[0][0], box.axis[0][1], box.axis[0][2]);
    printf("a1 %.1f %.1f %.1f \n", box.axis[1][0], box.axis[1][1], box.axis[1][2]);
    printf("a2 %.1f %.1f %.1f \n", box.axis[2][0], box.axis[2][1], box.axis[2][2]);
    printf("t  %.1f %.1f %.1f \n", box.center[0], box.center[1], box.center[2]);
    printf("---------------- \n");
}

void rs_sf_util_set_to_zeros(rs_sf_image* img);
void eigen_3x3_real_symmetric(float D[6], float u[3], float v[3][3]);
void rs_sf_util_convert_to_rgb_image(rs_sf_image* rgb, const rs_sf_image* src);
void rs_sf_util_draw_planes(rs_sf_image* rgb, const rs_sf_image* map, const rs_sf_image* src = nullptr, const unsigned char* rgb_table[3] = nullptr, int num_color = 0);
void rs_sf_util_scale_plane_ids(rs_sf_image* map, int max_pid);
void rs_sf_util_draw_line_rgb(rs_sf_image * rgb, v2 p0, v2 p1, const b3& color, const int size = 2);
void rs_sf_util_draw_boxes(rs_sf_image* rgb, const rs_sf_intrinsics& camera, const std::vector<rs_sf_box>& boxes);

std::vector<contour> find_contours_in_binary_img(rs_sf_image* bimg);
contour follow_border(uint8_t* pixel, const int w, const int h, const int _x0);


#endif // ! rs_sf_util_h

