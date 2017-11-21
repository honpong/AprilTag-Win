/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs_box_sdk.hpp
//  boxsdk
//
//  Created by Hon Pong (Gary) Ho
//
#pragma once


#ifndef rs_box_sdk_hpp
#define rs_box_sdk_hpp

#ifdef RS2_MEASURE_EXPORTS
#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
#define RS2_MEASURE_DECL __declspec(dllexport)
#else
#define RS2_MEASURE_DECL __attribute__((visibility("default")))
#endif
#else
#define RS2_MEASURE_DECL
#endif

#include "librealsense2/rs.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum rs2_measure_const
    {
        RS2_MEASURE_BOX_MAXCOUNT = 10
    } rs2_measure_const;

    struct rs2_measure_box
    {
        float center[3];   /**< box center in 3d world coordinate           */
        float axis[3][3];  /**< box axis and lengths in 3d world coordinate */
    };

    struct rs2_measure_camera_state
    {
        rs2_intrinsics* depth_intrinsics;
        rs2_intrinsics* color_intrinsics;
        float depth_pose[12];
        float color_pose[12];
    };

    struct rs2_box_measure;

    RS2_MEASURE_DECL void* rs2_box_measure_create(rs2_box_measure** box_measure, rs2_error** e);
    RS2_MEASURE_DECL void rs2_box_measure_delete(rs2_box_measure* box_measure, rs2_error** e);
    RS2_MEASURE_DECL void rs2_box_measure_reset(rs2_box_measure* box_measure, rs2_error** e);
    RS2_MEASURE_DECL void rs2_box_measure_set_depth_unit(rs2_box_measure* box_measure, float depth_unit, rs2_error** e);
    RS2_MEASURE_DECL int rs2_box_measure_get_boxes(rs2_box_measure* box_measure, rs2_measure_box* boxes, rs2_error** e);
    RS2_MEASURE_DECL const char* rs2_measure_get_realsense_icon(int* icon_width, int* icon_height, rs2_format* format, rs2_error** e);

#ifdef __cplusplus
}

#include <cfloat>
#include <cmath>
#include <iomanip>
namespace rs2
{
    inline std::string stri(float v, int p = 2) {
        std::ostringstream ss; ss << std::fixed << std::setprecision(p) << v << " "; return ss.str();
    }

    static void get_inverse_pose(const float pose[12], float inv_pose[12])
    {
        inv_pose[0] = pose[0]; inv_pose[1] = pose[4]; inv_pose[2] = pose[8];
        inv_pose[3] = -((pose[0] * pose[3]) + (pose[4] * pose[7]) + (pose[8] * pose[11]));
        inv_pose[4] = pose[1]; inv_pose[5] = pose[5]; inv_pose[6] = pose[9];
        inv_pose[7] = -((pose[1] * pose[3]) + (pose[5] * pose[7]) + (pose[9] * pose[11]));
        inv_pose[8] = pose[2]; inv_pose[9] = pose[6]; inv_pose[10] = pose[10];
        inv_pose[11] = -((pose[2] * pose[3]) + (pose[6] * pose[7]) + (pose[10] * pose[11]));
    }

    struct box : public rs2_measure_box
    {
        struct wire_frame { float end_pt[12][2][2]; };

        box(rs2_measure_box& ref) : rs2_measure_box(ref) {}

        inline float dim_sqr(int a) const /**< squared box dimension along axis a in meter */
        {
            return axis[a][0] * axis[a][0] + axis[a][1] * axis[a][1] + axis[a][2] * axis[a][2];
        }

        inline float dim(int a, float scale = 1000.0f) const /**< box dimension alogn axis a in meter */
        {
            return std::sqrt(dim_sqr(a)) * scale;
        }

        inline std::string str() const
        {
            return stri(dim(0), 0) + "x " + stri(dim(1), 0) + "x " + stri(dim(2), 0) + "mm^3";
        }

        wire_frame project_box_onto_frame(const rs2_intrinsics& cam, const float pose[12]) const
        {
            static const float line_idx[][2][3] = {
                { { -.5f,-.5f,-.5f },{ .5f,-.5f,-.5f } },{ { -.5f,-.5f,.5f },{ .5f,-.5f,.5f } },{ { -.5f,.5f,-.5f },{ .5f,.5f,-.5f } },{ { -.5f,.5f,.5f },{ .5f,.5f,.5f } },
                { { -.5f,-.5f,-.5f },{ -.5f,.5f,-.5f } },{ { -.5f,-.5f,.5f },{ -.5f,.5f,.5f } },{ { .5f,-.5f,-.5f },{ .5f,.5f,-.5f } },{ { .5f,-.5f,.5f },{ .5f,.5f,.5f } },
                { { -.5f,-.5f,-.5f },{ -.5f,-.5f,.5f } },{ { -.5f,.5f,-.5f },{ -.5f,.5f,.5f } },{ { .5f,-.5f,-.5f },{ .5f,-.5f,.5f } },{ { .5f,.5f,-.5f },{ .5f,.5f,.5f } } };

            wire_frame box_frame; float pt[3], inv_pose[12];
            get_inverse_pose(pose, inv_pose);
            for (int l = 0; l < 12; ++l) {
                for (int i = 0; i < 2; ++i) {
                    for (int d = 0; d < 3; ++d)
                        pt[d] = center[d] + axis[0][d] * line_idx[l][i][0] + axis[1][d] * line_idx[l][i][1] + axis[2][d] * line_idx[l][i][2];
                    const auto x = pt[0] * inv_pose[0] + pt[1] * inv_pose[1] + pt[2] * inv_pose[2] + inv_pose[3];
                    const auto y = pt[0] * inv_pose[4] + pt[1] * inv_pose[5] + pt[2] * inv_pose[6] + inv_pose[7];
                    const auto z = pt[0] * inv_pose[8] + pt[1] * inv_pose[9] + pt[2] * inv_pose[10] + inv_pose[11];
                    const auto iz = fabs(z) <= FLT_EPSILON ? 0 : 1.0f / z;
                    box_frame.end_pt[l][i][0] = x * cam.fx * iz + cam.ppx;
                    box_frame.end_pt[l][i][1] = y * cam.fy * iz + cam.ppy;
                }
            }
            return box_frame;
        }
    };

    class box_frameset : public frameset
    {
    public:
        box_frameset(frame& f) : frameset(f) {}

        const rs2_measure_camera_state* state() const { return (rs2_measure_camera_state*)((*this)[0].get_data()); }

        template<typename DRAW_FUNC>
        void draw_pose_text(int x, int y, int line_height, DRAW_FUNC draw_text) const {
            auto pose = state()->color_pose;
            draw_text(x, y + 0 * line_height, (stri(pose[0]) + stri(pose[1]) + stri(pose[2]) + stri(pose[3])).c_str());
            draw_text(x, y + 1 * line_height, (stri(pose[4]) + stri(pose[5]) + stri(pose[6]) + stri(pose[7])).c_str());
            draw_text(x, y + 2 * line_height, (stri(pose[8]) + stri(pose[9]) + stri(pose[10]) + stri(pose[11])).c_str());
        }

        box::wire_frame project_box_onto_color(const box& b) const {
            return b.project_box_onto_frame(*state()->color_intrinsics, state()->color_pose);
        }

        box::wire_frame project_box_onto_depth(const box& b) const {
            return b.project_box_onto_frame(*state()->depth_intrinsics, state()->depth_pose);
        }
    };

    class box_measure
    {
    public:

        typedef std::vector<box> box_vector;

        box_measure(device dev = device()) : _queue(1), _stream_w(0), _stream_h(0), _device(dev)
        {
            rs2_error* e = nullptr;

            _block = std::shared_ptr<processing_block>((processing_block*)rs2_box_measure_create(&_box_measure, &e));
            error::handle(e);

            if (_device.get()) { try_set_depth_scale(_device); }

            _block->start(_queue);
        }

        void reset()
        {
            rs2_error *e = nullptr;

            rs2_box_measure_reset(_box_measure, &e);
            error::handle(e);
        }

        config get_camera_config()
        {
            if (_camera_name == "Intel RealSense 410") {
                _stream_w = 1280 / 2; _stream_h = 960 / 2;
            }
            else if (_camera_name == "Intel RealSense 415") {
                _stream_w = 1280 / 2; _stream_h = 960 / 2;
            }
            else if (_camera_name == "Intel RealSense SR300") {
                _stream_w = 640; _stream_h = 480;
            }
            else {
                _stream_w = 640; _stream_h = 480;
                //throw std::runtime_error(_camera_name + " not supported by Box SDK!");
            }
            config config;
            config.enable_stream(RS2_STREAM_DEPTH, 0, _stream_w, _stream_h, RS2_FORMAT_Z16);
            config.enable_stream(RS2_STREAM_COLOR, 0, _stream_w, _stream_h, RS2_FORMAT_RGB8);

            return config;
        }

        int stream_w() const { return _stream_w; }
        int stream_h() const { return _stream_h; }

        box_frameset process(frameset frame)
        {
            (*_block)(frame);
            rs2::frame f;
            _queue.poll_for_frame(&f);
            return box_frameset(f);
        }

        box_vector get_boxes()
        {
            rs2_error* e = nullptr;
            int nbox = rs2_box_measure_get_boxes(_box_measure, _box, &e);
            error::handle(e);

            return box_vector(_box, _box + nbox);
        }

        bool try_set_depth_scale(device dev) try
        {
            rs2_error* e = nullptr;

            // get the device name
            _camera_name = dev.get_info(RS2_CAMERA_INFO_NAME);

            // Go over the device's sensors
            for (rs2::sensor& sensor : dev.query_sensors())
            {
                // Check if the sensor if a depth sensor
                if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
                {
                    rs2_box_measure_set_depth_unit(_box_measure, dpt.get_depth_scale(), &e);
                    error::handle(e);

                    return true;
                }
            }
            return false;
        }
        catch (...) { return false; }

        static const char* get_icon(int& w, int &h, rs2_format& format)
        {
            rs2_error* e = nullptr;
            auto icon = rs2_measure_get_realsense_icon(&w, &h, &format, &e);
            error::handle(e);
            return icon;
        }

    private:
        rs2::device _device;
        std::shared_ptr<processing_block> _block;
        frame_queue _queue;

        rs2_box_measure* _box_measure;
        rs2_measure_box _box[RS2_MEASURE_BOX_MAXCOUNT];
        std::string _camera_name;
        int _stream_w, _stream_h;
    };
}
#endif //__cplusplus

#endif /* rs_box_sdk_hpp */
