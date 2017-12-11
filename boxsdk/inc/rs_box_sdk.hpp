/*******************************************************************************
 
 Copyright 2015-2018 Intel Corporation.
 
 This software and the related documents are Intel copyrighted materials,
 and your use of them is governed by the express license under which
 they were provided to you. Unless the License provides otherwise,
 you may not use, modify, copy, publish, distribute, disclose or transmit
 this software or the related documents without Intel's prior written permission.
 
 This software and the related documents are provided as is, with no express or
 implied warranties, other than those that are expressly stated in the License.
 
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
        RS2_STREAM_POSE = 0,
        RS2_STREAM_PLANE = 3,
        RS2_MEASURE_BOX_MAXCOUNT = 10,
    } rs2_measure_const;

    struct rs2_measure_box
    {
        float center[3];   /**< box center in 3d world coordinate           */
        float axis[3][3];  /**< box axis and lengths in 3d world coordinate */
    };
    
    struct rs2_measure_box_wireframe
    {
        float end_pt[12][2][2]; /**< 12 lines by 12 pairs of 2d pixel coorindate */
    };
    
    struct rs2_measure_camera_state
    {
        rs2_intrinsics* intrinsics;
        float camera_pose[12];
    };

    struct rs2_box_measure; /**< box detector object */

    RS2_MEASURE_DECL void* rs2_box_measure_create(rs2_box_measure** box_measure, float depth_unit, rs2_error** e);
    RS2_MEASURE_DECL void rs2_box_measure_configure(rs2_box_measure* box_measure, const rs2_measure_const& out_stream, int flag, rs2_error** e);
    RS2_MEASURE_DECL void rs2_box_measure_reset(rs2_box_measure* box_measure, rs2_error** e);
    RS2_MEASURE_DECL int rs2_box_measure_get_boxes(rs2_box_measure* box_measure, rs2_measure_box* boxes, rs2_error** e);
    RS2_MEASURE_DECL void rs2_box_meausre_project_box_onto_frame(const rs2_measure_box* box, const rs2_measure_camera_state* camera, rs2_measure_box_wireframe* wireframe, rs2_error** e);
    RS2_MEASURE_DECL const char* rs2_measure_get_realsense_icon(int* icon_width, int* icon_height, rs2_format* format, rs2_error** e);

#ifdef __cplusplus
}

#include <cmath>
#include <iomanip>
namespace rs2
{
    static std::string f_str(float v, int p = 2) {
        std::ostringstream ss; ss << std::fixed << std::setprecision(p) << v; return ss.str();
    }

    static std::string stri(float v, int w, int p = 0) {
        std::string s0 = f_str(v, p), s = ""; while ((s + s0).length() < w) { s += " "; } return s + s0;
    }

    class box_frameset : public frameset
    {
    public:
        box_frameset(frame& f) : frameset(f) {}

        const rs2_measure_camera_state& state(const rs2_stream& s) const { return ((rs2_measure_camera_state*)((*this)[RS2_STREAM_POSE].get_data()))[s]; }

        operator bool() const { return size() > 3; }
    };

    struct box : public rs2_measure_box
    {
        typedef rs2_measure_box_wireframe wireframe;
        typedef rs2_measure_camera_state camera_state;

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
            return stri(dim(0), 4) + "x" + stri(dim(1), 4) + "x" + stri(dim(2), 4);
        }

        inline wireframe project_box_onto_frame(box_frameset& f, const rs2_stream& s) const
        {
            return project_box_onto_frame(f.state(s));
        }

        wireframe project_box_onto_frame(const rs2_measure_camera_state& camera) const
        {
            rs2_error* e = nullptr;

            wireframe box_frame;
            rs2_box_meausre_project_box_onto_frame(this, &camera, &box_frame, &e);
            error::handle(e);
            
            return box_frame;
        }
    };

    struct box_vector : public std::vector<box>
    {
        template<typename T>
        box_vector(T a, T b) : std::vector<box>(a, b) {}

        operator bool() const { return size() > 0; }
    };

    class box_measure
    {
    public:

        box_measure(device dev = device()) : _device(dev), _queue(1), _stream_w(0), _stream_h(0)
        {
            rs2_error* e = nullptr;
            
            float depth_unit = try_get_depth_scale(_device);

            printf("depth unit %f \n", depth_unit);

            _block = std::shared_ptr<processing_block>((processing_block*)rs2_box_measure_create(&_box_measure, depth_unit, &e));
            error::handle(e);

            _block->start(_queue);
        }

        void reset()
        {
            rs2_error *e = nullptr;

            rs2_box_measure_reset(_box_measure, &e);
            error::handle(e);
        }

        void configure(const rs2_measure_const& s, bool flag)
        {
            rs2_error *e = nullptr;
            rs2_box_measure_configure(_box_measure, s, flag ? 1 : 0, &e);
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
                _depth_unit = 0.00125f;
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

        float try_get_depth_scale(device dev) try
        {
            // get the device name
            _camera_name = dev.get_info(RS2_CAMERA_INFO_NAME);

            if (_depth_unit != 0.0f) return _depth_unit;

            // Go over the device's sensors
            for (rs2::sensor& sensor : dev.query_sensors())
            {
                // Check if the sensor if a depth sensor
                if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
                {
                    return _depth_unit = std::fmaxf(0.001f, dpt.get_depth_scale());
                }
            }
            return 0.001f;
        }
        catch (...) { return 0.001f; }

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
        float _depth_unit = 0.0f;
    };
}
#endif //__cplusplus

#endif /* rs_box_sdk_hpp */
