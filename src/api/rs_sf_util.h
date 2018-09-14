#pragma once
#ifndef rs_sf_util_h
#define rs_sf_util_h

#include "rs_shapefit.h"
#include <array>
#include <list>
#include <deque>
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <stdio.h>
#include <mutex>
#include <future>

//Don't use GPL-licensed pieces of eigen
#define EIGEN_MPL2_ONLY

//This disables internal asserts which slow eigen down quite a bit
#ifndef DEBUG
#define EIGEN_NO_DEBUG
#endif

#include <Eigen/Dense>
#if defined(OPENCV_FOUND) | defined(OpenCV_FOUND)
#include <opencv2/opencv.hpp>
#endif

static const int INVALID_PID = 0;
static const int MAX_VALID_PID = 254;
static const int PLANE_SRC_PID = 255;
static const float FLOAT_MAX_VALUE = std::numeric_limits<float>::max();
static const float FLOAT_MIN_VALUE = -FLOAT_MAX_VALUE;


template <int Rows = Eigen::Dynamic> using v = Eigen::Matrix<float, Rows, 1>;
template <int Rows = Eigen::Dynamic> using i = Eigen::Matrix<int, Rows, 1>;
typedef i<2> i2;
typedef i<4> i4;
typedef v<2> v2;
typedef v<3> v3;
typedef v<4> v4;
typedef Eigen::Quaternionf qv3;
typedef Eigen::Matrix<unsigned char, 3, 1> b3;
typedef Eigen::Matrix<float, 3, 3, Eigen::RowMajor> m3;
typedef Eigen::Map<v3> v3_map;
typedef Eigen::Map<Eigen::Matrix<float, 3, 3, Eigen::ColMajor>> m3_axis_map;
typedef std::vector<i2> contour;
typedef std::array<v3, 4> box_plane;

struct rs_shapefit
{
    virtual ~rs_shapefit() {}
    virtual rs_sf_status set_option(rs_sf_fit_option option, double value) { m_param[option] = value; return RS_SF_SUCCESS; }
    virtual rs_sf_status set_locked_inputs(const rs_sf_image *img) { return run_task(0) ? RS_SF_BUSY : (rs_sf_status)run_task(-1); }
    virtual rs_sf_status set_locked_outputs() { return m_task_status.valid() ? m_task_status.get() : RS_SF_INVALID_ARG; }
    bool run_task(long long ms) {
        if (!m_task_status.valid()) return false;
        if (ms >= 0) return m_task_status.wait_for(std::chrono::milliseconds(ms)) != std::future_status::ready;
        set_locked_outputs(); return false;
    }
    std::mutex m_input_mutex;
    std::future<rs_sf_status> m_task_status;
#ifndef _DEBUG
    double m_param[RS_SF_OPTION_COUNT] = { 6,0 };
#else 
    double m_param[RS_SF_OPTION_COUNT] = { -1,0 };
#endif
    rs_sf_intrinsics m_intrinsics;

    enum fit_option_tracking { CONTINUE = 0, SINGLE_FRAME = 1 };
    enum fit_option_get_plane_id { ORIGINAL = 0, SCALED = 1, REMAP = 2 };
    long long get_option_async_process_wait() const { return (long long)m_param[RS_SF_OPTION_ASYNC_WAIT]; }
    fit_option_tracking get_option_track() const { return (fit_option_tracking)(int)m_param[RS_SF_OPTION_TRACKING]; }
    fit_option_get_plane_id get_option_get_plane_id() const { return (fit_option_get_plane_id)(int)m_param[RS_SF_OPTION_GET_PLANE_ID]; }

    static std::chrono::time_point<std::chrono::steady_clock> now() { return std::chrono::steady_clock::now(); }
    template<typename T0, typename T1> static float abs_time_diff_ms(const T0& t0, const T1& t1) {
        return std::abs(std::chrono::duration<float, std::milli>(t0 - t1).count());
    }
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
void rs_sf_util_convert_to_rgb_image(rs_sf_image* rgb, const rs_sf_image* src);
void rs_sf_util_copy_depth_image(rs_sf_image_depth& dst, const rs_sf_image* src);
void rs_sf_util_copy_color_image(rs_sf_image_rgb& dst, const rs_sf_image* src);
void rs_sf_util_draw_plane_ids(rs_sf_image* rgb, const rs_sf_image* map, bool overwrite_rgb = false, const unsigned char(*rgb_table)[3] = nullptr);
void rs_sf_util_scale_plane_ids(rs_sf_image* map, int max_pid);
void rs_sf_util_remap_plane_ids(rs_sf_image * map);
void rs_sf_util_draw_line_rgb(rs_sf_image * rgb, const v2& p0, const v2& p1, const b3& color, const int size = 4);
void rs_sf_util_draw_plane_contours(rs_sf_image *rgb, const pose_t& pose, const rs_sf_intrinsics& camera, const rs_sf_plane planes[RS_SF_MAX_PLANE_COUNT], const int pt_per_line =1);
void rs_sf_util_draw_boxes(rs_sf_image* rgb, const pose_t& pose, const rs_sf_intrinsics& camera, const std::vector<rs_sf_box>& boxes, const b3& color);
box_plane rs_sf_util_get_box_plane(const rs_sf_box& box, int plane_id);
void rs_sf_util_raycast_boxes(rs_sf_image * depth, const pose_t& pose, const float depth_unit_in_meter, const rs_sf_intrinsics& camera, const std::vector<rs_sf_box>& boxes);
rs_sf_intrinsics rs_sf_util_match_intrinsics(const rs_sf_image* img, const rs_sf_intrinsics& ref);
inline int rs_sf_util_image_to_line_width(const rs_sf_image *img) { return std::max(1, img->img_w / 320); }

void eigen_3x3_real_symmetric(float D[6], float u[3], float v[3][3]);

std::vector<contour> find_contours_in_binary_img(rs_sf_image* bimg);
contour follow_border(uint8_t* pixel, const int w, const int h, const int _x0);

std::vector<std::vector<int>> find_contours_in_map_uchar(short* map, const int w, const int h, const int min_len);
bool try_follow_border_uchar(std::vector<std::vector<int>>& dst_list, short* map, const int w, const int h, const int _x0, const int min_len);

#endif // ! rs_sf_util_h

