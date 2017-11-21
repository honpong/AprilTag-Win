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
#include "rs_shapefit.h"
#include "rs_box_api.h"
#include "rs_icon.h"
#include "rs_sf_pose_tracker.h"

namespace rs2
{
    static rs_sf_image& operator<<(rs_sf_image& img, video_frame& f)
    {
        static const int stream_type_to_byte[] = { 1,2,3,1,1,1,1,1 };
        img.byte_per_pixel = stream_type_to_byte[f.get_profile().stream_type()];
        img.img_h = f.get_height();
        img.img_w = f.get_width();
        img.frame_id = f.get_frame_number();
        img.data = reinterpret_cast<uint8_t*>(const_cast<void *>(f.get_data()));
        return img;
    }
    static rs_sf_image& operator<<(rs_sf_image& img, rs2_intrinsics* intrinsics) { img.intrinsics = (rs_sf_intrinsics*)intrinsics; return img; }
    static rs_sf_image& operator<<(rs_sf_image& img, float* cam_pose) { img.cam_pose = cam_pose; return img; }
    static rs_sf_image& operator<<(rs_sf_image& img, unsigned long long frame_id) { img.frame_id = frame_id; return img; }
    static rs_sf_image& operator<<(rs_sf_image& img, const void* data) { img.data = reinterpret_cast<unsigned char*>(const_cast<void*>(data)); return img; }

    struct sp_handle
    {
        sp_handle(const rs2_intrinsics* i, int r_width = 320, int r_height = 240) : sp_init(
            rs_sf_setup_scene_perception(i->fx, i->fy, i->ppx, i->ppy, i->width, i->height, r_width, r_height, RS_SF_MED_RESOLUTION)),
            _sdepth(r_width * r_height * (sp_init ? 1 : 0)), _scolor(r_width * r_height * (sp_init ? 3 : 0)) {}
        ~sp_handle() { if (sp_init) rs_sf_pose_tracking_release(); }

        bool sp_init, was_tracking = false;
        std::unique_ptr<float[]> buf;
        std::vector<unsigned short> _sdepth;
        std::vector<unsigned char> _scolor;

        bool track(rs_sf_image& depth_frame, rs_sf_image& color_frame, const rs2_extrinsics& d2c, bool reset_request)
        {
            if (!sp_init) {
                memcpy(depth_frame.cam_pose, std::vector<float>{ 1.0f, .0f, .0f, .0f, .0f, 1.0f, .0f, .0f, .0f, .0f, 1.0f, .0f }.data(), sizeof(float) * 12);
                depth_frame.cam_pose = nullptr;
                return true;
            }

            //start_time = std::chrono::steady_clock::now();
            //auto start_pose_track_time = std::chrono::steady_clock::now();

            // down-sample depth
            auto depth_data = (unsigned short*)depth_frame.data;
            auto color_data = color_frame.data;
            auto s_depth_data = _sdepth.data();
            auto s_color_data = _scolor.data();
            for (int y = 0, p = 0, p3 = 0, w = depth_frame.img_w, h = depth_frame.img_h; y < 240; ++y) {
                for (int x = 0; x < 320; ++x, ++p, p3 += 3) {
                    const int p_src = (y * h / 240) * w + (x * w / 320);
                    s_depth_data[p] = depth_data[p_src];
                    memcpy(s_color_data + p3, color_data + p3, 3);
                }
            }

            // do pose tracking
            bool track_success = rs_sf_do_scene_perception_tracking(_sdepth.data(), _scolor.data(), reset_request, depth_frame.cam_pose);
            bool cast_success = rs_sf_do_scene_perception_ray_casting(320, 240, _sdepth.data(), buf);
            bool switch_track = (track_success && cast_success) != was_tracking;

            // up-sample depth
            if (was_tracking = (track_success && cast_success)) {
                for (int y = 0, p = 0, h = depth_frame.img_h, w = depth_frame.img_w; y < h; ++y) {
                    for (int x = 0; x < w; ++x, ++p) {
                        depth_data[p] = s_depth_data[(y * 240 / h) * 320 + (x * 320 / w)];
                    }
                }
                auto* d = depth_frame.cam_pose, *c = color_frame.cam_pose;
                auto& r = d2c.rotation; auto& t = d2c.translation;
                c[0] = r[0] * d[0] + r[1] * d[4] + r[2] * d[8];
                c[1] = r[0] * d[1] + r[1] * d[5] + r[2] * d[9];
                c[2] = r[0] * d[2] + r[1] * d[6] + r[2] * d[10];
                c[3] = r[0] * d[3] + r[1] * d[7] + r[2] * d[11] + t[0];
                c[4] = r[3] * d[0] + r[4] * d[4] + r[5] * d[8];
                c[5] = r[3] * d[1] + r[4] * d[5] + r[5] * d[9];
                c[6] = r[3] * d[2] + r[4] * d[6] + r[5] * d[10];
                c[7] = r[3] * d[3] + r[4] * d[7] + r[5] * d[11] + t[1];
                c[8] = r[6] * d[0] + r[7] * d[4] + r[8] * d[8];
                c[9] = r[6] * d[1] + r[7] * d[5] + r[8] * d[9];
                c[10] = r[6] * d[2] + r[7] * d[6] + r[8] * d[10];
                c[11] = r[6] * d[3] + r[7] * d[7] + r[8] * d[11] + t[2];
            }
            return was_tracking;
        }
    };

    struct box_measure_impl
    {
        virtual ~box_measure_impl() {}

        rs_shapefit* init_box_detector(video_frame& input_depth_frame, video_frame& input_color_frame)
        {
            _depth_stream_profile = std::make_shared<video_stream_profile>(input_depth_frame.get_profile());
            _depth_intrinsics = _depth_stream_profile->get_intrinsics();
            auto box_detector = (_detector = std::shared_ptr<rs_shapefit>(rs_shapefit_create((rs_sf_intrinsics*)&_depth_intrinsics, RS_SHAPEFIT_BOX),
                rs_shapefit_delete)).get();

            set_depth_unit(_depth_unit);
            rs_shapefit_set_option(box_detector, RS_SF_OPTION_ASYNC_WAIT, 500.0);
            //rs_shapefit_set_option(_detector.get(), RS_SF_OPTION_PLANE_NOISE, 2);
            //rs_shapefit_set_option(_detector.get(), RS_SF_OPTION_PLANE_RES, 1);

            _color_stream_profile = std::make_shared<video_stream_profile>(input_color_frame.get_profile());
            _color_intrinsics = _color_stream_profile->get_intrinsics();
            _depth_to_color = _color_stream_profile->get_extrinsics_to(*_depth_stream_profile);

            for (auto i : { 0, 1, 2, 9, 3, 4, 5, 10, 6, 7, 8, 11 })
                color_image_pose.push_back(((float*)&_depth_to_color)[i]);

            for (auto v : { 1.0f, .0f, .0f, .0f, .0f, 1.0f, .0f, .0f, .0f, 0.f, 1.0f, 0.f })
                depth_image_pose.push_back(v);
            depth_image_pose = { 1.0f, .0f, .0f, .0f, .0f, 1.0f, .0f, .0f, .0f, 0.f, 1.0f, 0.f };

            _image[BOX_SRC_DEPTH] << input_depth_frame << &_depth_intrinsics << depth_image_pose.data();
            _image[BOX_SRC_COLOR] << input_color_frame << &_color_intrinsics << color_image_pose.data();
            _image[BOX_DST_COLOR] << input_color_frame << &_depth_intrinsics;

            if (_sp) _sp.reset();
            _sp = std::make_unique<sp_handle>(&_depth_intrinsics);

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
                _image[BOX_SRC_DEPTH] << input_depth_frame.get_frame_number() << input_depth_frame.get_data() << depth_image_pose.data();
                _image[BOX_SRC_COLOR] << input_color_frame.get_frame_number() << input_color_frame.get_data();
            }

            if (_sp) { _reset_request |= !_sp->track(_image[BOX_SRC_DEPTH], _image[BOX_SRC_COLOR], _depth_to_color, _reset_request); }
            rs_shapefit_set_option(box_detector, RS_SF_OPTION_TRACKING, !_reset_request ? 0 : 1);
            _reset_request = false;

            auto status = rs_shapefit_depth_image(box_detector, &_image[BOX_SRC_DEPTH]);
            if (status == RS_SF_SUCCESS)
            {
                auto output_color_frame = src.allocate_video_frame(*_color_stream_profile, input_color_frame);
                auto output_align_frame = src.allocate_video_frame(*_color_stream_profile, input_color_frame, 8, 1, sizeof(rs2_measure_camera_state), 1);

                _image[BOX_DST_COLOR] << input_color_frame.get_frame_number() << output_color_frame.get_data();

                auto cameras = (rs2_measure_camera_state*)output_align_frame.get_data();
                cameras->color_intrinsics = &_color_intrinsics;
                cameras->depth_intrinsics = &_depth_intrinsics;
                memcpy(cameras->depth_pose, depth_image_pose.data(), sizeof(float)*depth_image_pose.size());
                memcpy(cameras->color_pose, color_image_pose.data(), sizeof(float)*color_image_pose.size());

                //rs_sf_planefit_draw_planes(box_detector, &_image[BOX_DST_COLOR]);
                //rs_sf_boxfit_draw_boxes(box_detector, &_image[BOX_DST_COLOR]);

                auto output = src.allocate_composite_frame({ output_align_frame, output_color_frame });
                src.frame_ready(std::move(output));
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

        void set_depth_unit(float depth_unit)
        {
            std::lock_guard<std::recursive_mutex> lock(_mutex);
            _depth_unit = depth_unit;
            if (_detector) { rs_shapefit_set_option(_detector.get(), RS_SF_OPTION_DEPTH_UNIT, _depth_unit); }
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

        processing_block* _host = nullptr;

    private:
        std::recursive_mutex _mutex;

        enum { BOX_SRC_DEPTH, BOX_SRC_COLOR, BOX_DST_DEPTH, BOX_DST_COLOR, BOX_IMG_COUNT };
        rs_sf_image _image[BOX_IMG_COUNT] = {};

        rs2_intrinsics _depth_intrinsics, _color_intrinsics;
        rs2_extrinsics _depth_to_color;
        std::shared_ptr<video_stream_profile> _depth_stream_profile, _color_stream_profile;

        std::unique_ptr<sp_handle> _sp;

        std::shared_ptr<rs_shapefit> _detector;
        std::vector<float> depth_image_pose, color_image_pose;
        float _depth_unit = 0.001f;
        bool _reset_request = false;
    };
}

struct rs2_box_measure : public rs2::box_measure_impl {};

void* rs2_box_measure_create(rs2_box_measure** box_measure, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);

    auto ptr = std::make_shared<rs2_box_measure>();
    *box_measure = ptr.get();
    return ptr->set_host(new rs2::processing_block([ptr = ptr](rs2::frame f, const rs2::frame_source& src) { (*ptr)(f, src); }));
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, box_measure)

void rs2_box_measure_delete(rs2_box_measure* box_measure, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);

    delete ((rs2::box_measure_impl*)box_measure)->_host;
}
HANDLE_EXCEPTIONS_AND_RETURN(, box_measure)

void rs2_box_measure_reset(rs2_box_measure* box_measure, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);

    ((rs2::box_measure_impl*)box_measure)->set_reset_request();
}
HANDLE_EXCEPTIONS_AND_RETURN(, box_measure)

void rs2_box_measure_set_depth_unit(rs2_box_measure* box_measure, float depth_unit, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);
    VALIDATE_RANGE(depth_unit, 0.00001f, 1.0f);

    box_measure->set_depth_unit(depth_unit);
}
HANDLE_EXCEPTIONS_AND_RETURN(, box_measure, depth_unit)

int rs2_box_measure_get_boxes(rs2_box_measure * box_measure, rs2_measure_box * boxes, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(box_measure);
    VALIDATE_NOT_NULL(boxes);

    return ((rs2::box_measure_impl*)box_measure)->get_boxes(boxes);
}
HANDLE_EXCEPTIONS_AND_RETURN(-1, box_measure, boxes)

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
