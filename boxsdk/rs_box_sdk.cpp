/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs_box_sdk.cpp
//  boxsdk
//
//  Created by Hon Pong (Gary) Ho
//
#define RS2_MEASURE_EXPORTS

#include <stdio.h>
#include "inc/rs_box_sdk.hpp"
#include "rs_box_image.hpp"
#include "rs_box_api.h"
#include "rs_icon.h"
#include "rs_sf_pose_tracker.h"

struct rs2_box_measure {};

namespace rs2
{ 
    struct box_measure_impl : public rs2_box_measure
    {
        enum { EXPORT_STATE, BOX_SRC_DEPTH, BOX_SRC_COLOR, BOX_DST_DENSE, BOX_DST_PLANE, BOX_DST_RAYCA, BOX_DST_COLOR, BOX_IMG_COUNT };
        box_measure_impl(float depth_unit = 0.001f, rs2_intrinsics intrinsics[2] = nullptr, rs2_extrinsics* c2d = nullptr) : _depth_unit(depth_unit)
        {
            if (intrinsics) { _custom_intrinsics = true; _depth_intrinsics = intrinsics[0]; _color_intrinsics = intrinsics[1]; }
            if (c2d) { _custom_extrinsics = true; _color_to_depth = *c2d; }
        }
        virtual ~box_measure_impl() {}

        void reset_poses()
        {
            _depth_image_pose.set_identity();
            _color_image_pose.clear();
            for (auto i : { 0, 1, 2, 9, 3, 4, 5, 10, 6, 7, 8, 11 })
                _color_image_pose.push_back(((float*)&_color_to_depth)[i]);
        }

        rs_shapefit* init_box_detector(video_frame& input_depth_frame, video_frame& input_color_frame)
        {
            _depth_stream_profile = std::make_shared<video_stream_profile>(input_depth_frame.get_profile());
            _color_stream_profile = std::make_shared<video_stream_profile>(input_color_frame.get_profile());
            if (!_custom_intrinsics) { _depth_intrinsics = _depth_stream_profile->get_intrinsics(); }
            if (!_custom_intrinsics) { _color_intrinsics = _color_stream_profile->get_intrinsics(); }
            if (!_custom_extrinsics) {
                try { _color_to_depth = _color_stream_profile->get_extrinsics_to(*_depth_stream_profile); }
                catch (...) {
                    printf("rs_box_sdk warning! depth to color extrinsics unavailable. \n");
                    set_identity(_color_to_depth);
                    _color_to_depth.translation[0] = -0.015f;
                }
            }

            reset_poses();
            _image[BOX_SRC_DEPTH] << input_depth_frame << &_depth_intrinsics << _depth_image_pose.data();
            _image[BOX_SRC_COLOR] << input_color_frame << &_color_intrinsics << _color_image_pose.data();
            _image[BOX_DST_DENSE] << input_depth_frame << &_depth_intrinsics << _depth_image_pose.data();
            _image[BOX_DST_PLANE] << input_color_frame << &_depth_intrinsics;
            _image[BOX_DST_RAYCA] << input_depth_frame << &_depth_intrinsics;
            _image[BOX_DST_COLOR] << input_color_frame << &_color_intrinsics << _color_image_pose.data();

            auto box_detector = (_detector = std::shared_ptr<rs_shapefit>(rs_shapefit_create((rs_sf_intrinsics*)&_depth_intrinsics, RS_SHAPEFIT_BOX),
                rs_shapefit_delete)).get();
            rs_shapefit_set_option(box_detector, RS_SF_OPTION_DEPTH_UNIT, _depth_unit);
            rs_shapefit_set_option(box_detector, RS_SF_OPTION_ASYNC_WAIT, 0.0);
            rs_shapefit_set_option(box_detector, RS_SF_OPTION_PLANE_NOISE, 1);
            //rs_shapefit_set_option(box_detector, RS_SF_OPTION_PLANE_RES, 1);

            if (_camera_tracker) _camera_tracker.reset();
            _camera_tracker = std::make_unique<camera_tracker>(&_depth_intrinsics, RS_SF_MED_RESOLUTION);
            _is_export[BOX_DST_DENSE] = _camera_tracker->is_valid();
            
            return box_detector;
        }

        void operator()(frame f, const frame_source& src)
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);

            auto fs = f.as<frameset>();
            auto input_depth_frame = fs.get_depth_frame();
            auto input_color_frame = fs.get_color_frame();
            auto box_detector = _detector.get();

            if (!box_detector) { box_detector = init_box_detector(input_depth_frame, input_color_frame); }
            else {
                reset_poses();
                _image[BOX_SRC_DEPTH] << input_depth_frame.get_frame_number() << input_depth_frame.get_data() << _depth_image_pose.data();
                _image[BOX_SRC_COLOR] << input_color_frame.get_frame_number() << input_color_frame.get_data();
                _image[BOX_DST_DENSE] << input_depth_frame.get_frame_number() << _depth_image_pose.data();
                _image[BOX_DST_PLANE] << input_depth_frame.get_frame_number();
                _image[BOX_DST_RAYCA] << input_depth_frame.get_frame_number() << _depth_image_pose.data();
                _image[BOX_DST_COLOR] << input_color_frame.get_frame_number() << _color_image_pose.data();
            }

            std::vector<rs2::frame> export_frame = {
                src.allocate_video_frame(*_depth_stream_profile, input_depth_frame, 8, 1, sizeof(rs2_measure_camera_state)*BOX_IMG_COUNT, 1),
                input_depth_frame,
                input_color_frame,
                !_is_export[BOX_DST_DENSE] ? input_depth_frame : src.allocate_video_frame(*_depth_stream_profile, input_depth_frame),
                !_is_export[BOX_DST_PLANE] ? input_color_frame : src.allocate_video_frame(*_color_stream_profile, input_color_frame),
                !_is_export[BOX_DST_RAYCA] ? input_depth_frame : src.allocate_video_frame(*_depth_stream_profile, input_depth_frame),
                !_is_export[BOX_DST_COLOR] ? input_color_frame : src.allocate_video_frame(*_color_stream_profile, input_color_frame),
            };

            for (auto s : { BOX_SRC_DEPTH, BOX_SRC_COLOR, BOX_DST_DENSE, BOX_DST_PLANE, BOX_DST_RAYCA, BOX_DST_COLOR })
                _image[s] << export_frame[s].get_data();

            if (_camera_tracker && _is_export[BOX_DST_DENSE]) {
                _reset_request |= !_camera_tracker->track(_image[BOX_SRC_DEPTH], _image[BOX_SRC_COLOR], _image[BOX_DST_DENSE], _color_to_depth, _reset_request);
            }
            rs_shapefit_set_option(box_detector, RS_SF_OPTION_TRACKING, !_reset_request ? 0 : 1);
            _reset_request = false;

            if (rs_shapefit_depth_image(box_detector, &_image[BOX_DST_DENSE]) >= RS_SF_SUCCESS)
            {
                for (auto s : { BOX_SRC_DEPTH, BOX_SRC_COLOR, BOX_DST_DENSE, BOX_DST_PLANE, BOX_DST_RAYCA, BOX_DST_COLOR }) {
                    if (_is_export[s]) { _state[s] << _image[s]; }
                }

                if (_is_export[BOX_DST_PLANE]) { rs_sf_planefit_draw_planes(box_detector, &_image[BOX_DST_PLANE]); }
                if (_is_export[BOX_DST_COLOR]) { rs_sf_boxfit_draw_boxes(box_detector, &_image[BOX_DST_COLOR], &_image[BOX_SRC_COLOR]); }
                if (_is_export[BOX_DST_RAYCA]) { rs_sf_boxfit_raycast_boxes(box_detector, nullptr, -1, &_image[BOX_DST_RAYCA]); }

                memcpy((void*)export_frame[EXPORT_STATE].get_data(), _state, sizeof(rs2_measure_camera_state)*BOX_IMG_COUNT);
                src.frame_ready(src.allocate_composite_frame(export_frame));
            }
        }

        int get_boxes(rs2_measure_box box[]) {

            std::lock_guard<std::recursive_mutex> lock(_mutex);
            int _num_box = 0;
            for (auto box_detector = _detector.get(); _num_box < RS2_MEASURE_BOX_MAXCOUNT; ++_num_box)
                if (rs_sf_boxfit_get_box(box_detector, _num_box, (rs_sf_box*)box + _num_box) != RS_SF_SUCCESS)
                    break;
            return _num_box;
        }

        void set(const int& s, const double flag)
        {
            _is_export[s] = (flag > 0.0);
        }

        void set_reset_request()
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            _reset_request = true;
        }

        processing_block* set_host(processing_block* host)
        {
            return _host = host;
        }

        void raycast_box_onto_frame(const rs2_measure_box& box, const rs2_measure_camera_state& cam, video_frame& depth_image, bool reset)
        {
            rs_sf_image depth; depth << depth_image;
            depth.cam_pose = (float*)cam.camera_pose;
            depth.intrinsics = (rs_sf_intrinsics*)cam.intrinsics;
            rs_sf_boxfit_raycast_boxes(_detector.get(), (rs_sf_box*)&box, &depth, reset ? nullptr : &depth);
        }

    private:
        
        // processing block operations
        std::recursive_mutex _mutex;
        bool _reset_request = false;
        processing_block* _host = nullptr;

        // image headers
        bool _is_export[BOX_IMG_COUNT] = { true, true, true, true, false, false, false };
        rs_sf_image _image[BOX_IMG_COUNT] = {};
        rs2_measure_camera_state _state[BOX_IMG_COUNT] = {};

        // stream related
        bool _custom_intrinsics = false, _custom_extrinsics = false;
        rs2_intrinsics _depth_intrinsics, _color_intrinsics;
        rs2_extrinsics _color_to_depth;
        std::shared_ptr<video_stream_profile> _depth_stream_profile, _color_stream_profile;

        // algorithm related
        std::unique_ptr<camera_tracker> _camera_tracker;
        std::shared_ptr<rs_shapefit> _detector;
        rs_sf_pose_data _depth_image_pose, _color_image_pose;
        float _depth_unit;
    };
}

void* rs2_box_measure_create(rs2_box_measure** box_measure, float depth_unit, rs2_intrinsics custom_intrinsics[2], rs2_extrinsics* custom_extrinsics, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);
    VALIDATE_RANGE(depth_unit, 0.00001f, 1.0f);

    auto ptr = std::make_shared<rs2::box_measure_impl>(depth_unit, custom_intrinsics, custom_extrinsics);
    *box_measure = ptr.get();
    return ptr->set_host(new rs2::processing_block([ptr = ptr](rs2::frame f, const rs2::frame_source& src) { (*ptr)(f, src); }));
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, box_measure, depth_unit)

void rs2_box_measure_configure(rs2_box_measure* box_measure, const rs2_measure_const out_stream, double flag, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);
    VALIDATE_RANGE(out_stream, rs2::box_measure_impl::BOX_DST_DENSE, rs2::box_measure_impl::BOX_IMG_COUNT - 1);

    ((rs2::box_measure_impl*)box_measure)->set(out_stream, flag);
}
HANDLE_EXCEPTIONS_AND_RETURN(, box_measure, out_stream, flag)

void rs2_box_measure_reset(rs2_box_measure* box_measure, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);

    ((rs2::box_measure_impl*)box_measure)->set_reset_request();
}
HANDLE_EXCEPTIONS_AND_RETURN(, box_measure)

int rs2_box_measure_get_boxes(rs2_box_measure * box_measure, rs2_measure_box * boxes, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);
    VALIDATE_NOT_NULL(boxes);

    return ((rs2::box_measure_impl*)box_measure)->get_boxes(boxes);
}
HANDLE_EXCEPTIONS_AND_RETURN(-1, box_measure, boxes)

void rs2_box_meausre_project_box_onto_frame(const rs2_measure_box* box, const rs2_measure_camera_state* camera, rs2_measure_box_wireframe* wireframe, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box);
    VALIDATE_NOT_NULL(camera);
    VALIDATE_NOT_NULL(wireframe);
    
    rs2::project_box_onto_frame(*box, *camera, *wireframe);
}
HANDLE_EXCEPTIONS_AND_RETURN(, box, camera, wireframe)

void rs2_box_measure_raycast_box_onto_frame(const rs2_box_measure * box_measure, const rs2_measure_box* box, const rs2_measure_camera_state* camera, rs2_frame* img, double reset, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);
    VALIDATE_NOT_NULL(box);
    VALIDATE_NOT_NULL(camera);
    VALIDATE_NOT_NULL(img);

    ((rs2::box_measure_impl*)box_measure)->raycast_box_onto_frame(*box, *camera, rs2::video_frame(rs2::frame(img)), reset == 1);
}
HANDLE_EXCEPTIONS_AND_RETURN(, box_measure, box, camera, img, reset)

const char* rs2_measure_get_realsense_icon(int* icon_width, int* icon_height, rs2_format* format, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(icon_width);
    VALIDATE_NOT_NULL(icon_height);
    VALIDATE_NOT_NULL(format);

    *icon_width = get_icon_width(realsense);
    *icon_height = get_icon_height(realsense);
    *format = RS2_FORMAT_BGRA8;
    return get_icon_data(realsense);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, icon_width, icon_height, format)
