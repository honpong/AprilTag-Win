#pragma once
#ifndef rs_sf_util_h
#define rs_sf_util_h

#include "rs_shapefit.h"
#include <list>
#include <vector>
#include <memory>
#include <iostream>
#include <stdio.h>
#include <Eigen/Dense>
#if defined(OPENCV_FOUND) | defined(OpenCV_FOUND)
#include <opencv2/opencv.hpp>
#endif

static const int MAX_VALID_PID = 254;
static const int PLANE_SRC_PID = 255;
static const float FLOAT_MAX_VALUE = std::numeric_limits<float>::max();
static const float FLOAT_MIN_VALUE = -FLOAT_MAX_VALUE;

typedef Eigen::Vector2i i2;
typedef Eigen::Vector4i i4;
typedef Eigen::Vector2f v2;
typedef Eigen::Vector3f v3;
typedef Eigen::Vector4f v4;
typedef Eigen::Matrix<unsigned char, 3, 1> b3;
typedef Eigen::Matrix<float, 3, 3, Eigen::RowMajor> m3;
typedef Eigen::Map<v3> v3_map;
typedef Eigen::Map<Eigen::Matrix<float, 3, 3, Eigen::ColMajor>> m3_axis_map;
typedef Eigen::Quaternionf qv3;
typedef std::vector<i2> contour;

#include <mutex>
#include <thread>
#include <future>
struct rs_shapefit
{
    virtual ~rs_shapefit() {}
    virtual rs_sf_status set_option(rs_sf_fit_option option, double value) { m_param[option] = value; return RS_SF_SUCCESS; }
    virtual rs_sf_status set_locked_new_inputs(const rs_sf_image *img) { return run_task(0) ? RS_SF_BUSY : RS_SF_SUCCESS; }
    bool run_task(long long ms) const {
        return m_task_status.valid() &&
            (m_task_status.wait_for(std::chrono::milliseconds(ms >= 0 ? ms : 0xffffffffffffff)) != std::future_status::ready);
    }
    std::mutex m_input_mutex;
    std::future<rs_sf_status> m_task_status;
    double m_param[RS_SF_OPTION_COUNT] = { 3,0 };
    rs_sf_intrinsics m_intrinsics;    

    enum fit_option_tracking { CONTINUE = 0, SINGLE_FRAME = 1 };
    enum fit_option_draw_planes { OVERLAY = 0, OVERWRITE = 1 };
    enum fit_option_get_plane_id { ORIGINAL = 0, SCALED = 1, REMAP = 2 };
    long long get_option_max_process_delay() const { return (long long)m_param[RS_SF_OPTION_MAX_PROCESS_DELAY]; }
    fit_option_tracking get_option_track() const { return (fit_option_tracking)(int)m_param[RS_SF_OPTION_TRACKING]; }
    fit_option_draw_planes get_option_draw_planes() const { return (fit_option_draw_planes)(int)m_param[RS_SF_OPTION_DRAW_PLANES]; }
    fit_option_get_plane_id get_option_get_plane_id() const { return (fit_option_get_plane_id)(int)m_param[RS_SF_OPTION_GET_PLANE_ID]; }
};


struct pose_t
{
    m3 rotation;
    v3 translation;

    inline pose_t& set_pose(const float p[12] = nullptr) {
		if (p) {
			rotation << p[0], p[1], p[2], p[4], p[5], p[6], p[8], p[9], p[10];
			translation << p[3] * 1000.0f, p[7] * 1000.0f, p[11] * 1000.0f;
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
void rs_sf_util_convert_to_rgb_image(rs_sf_image* rgb, const rs_sf_image* src);
void rs_sf_util_copy_depth_image(rs_sf_image_depth& dst, const rs_sf_image* src);
void rs_sf_util_draw_planes(rs_sf_image* rgb, const rs_sf_image* map, const rs_sf_image* src = nullptr, bool overwrite_rgb = false, const unsigned char* rgb_table[3] = nullptr, int num_color = 0);
void rs_sf_util_scale_plane_ids(rs_sf_image* map, int max_pid);
void rs_sf_util_remap_plane_ids(rs_sf_image * map);
void rs_sf_util_draw_line_rgb(rs_sf_image * rgb, v2 p0, v2 p1, const b3& color, const int size = 2);
void rs_sf_util_draw_boxes(rs_sf_image* rgb, const pose_t& pose, const rs_sf_intrinsics& camera, const std::vector<rs_sf_box>& boxes);

void eigen_3x3_real_symmetric(float D[6], float u[3], float v[3][3]);

std::vector<contour> find_contours_in_binary_img(rs_sf_image* bimg);
contour follow_border(uint8_t* pixel, const int w, const int h, const int _x0);


#endif // ! rs_sf_util_h

