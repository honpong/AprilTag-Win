/*******************************************************************************
 
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2017 Intel Corporation. All Rights Reserved.
 
 *******************************************************************************/
#pragma once
#include <memory>
#include "librealsense2/rs.hpp"
#include "rs_shapefit.h"

enum rs_sf_pose_track_resolution : int
{
    RS_SF_LOW_RESOLUTION = 0,
    RS_SF_MED_RESOLUTION = 1,
    RS_SF_HIGH_RESOLUTION = 2
};

bool rs_sf_setup_scene_perception(float rfx, float rfy, float rpx, float rpy, unsigned int rw, unsigned int rh,
                                  int target_width,
                                  int target_height,
                                  rs_sf_pose_track_resolution resolution);

bool rs_sf_do_scene_perception_tracking(unsigned short* depth_data,
                                        unsigned char* rgb_data,
                                        bool& reset_request,
                                        float cam_pose[12] = nullptr);

bool rs_sf_do_scene_perception_ray_casting(int image_width,
                                           int image_height,
                                           unsigned short* dst_depth_data,
                                           std::unique_ptr<float[]>& buffer);

void rs_sf_pose_tracking_release();

namespace rs2
{
    struct camera_tracker
    {
        camera_tracker(const rs2_intrinsics* i, int r_width = 320, int r_height = 240) :
        _sp_init(rs_sf_setup_scene_perception(i->fx, i->fy, i->ppx, i->ppy, i->width, i->height, r_width, r_height, RS_SF_MED_RESOLUTION)),
        _sdepth(r_width * r_height * (_sp_init ? 1 : 0)),
        _scolor(r_width * r_height * (_sp_init ? 3 : 0)) {}
        
        ~camera_tracker() { if (_sp_init) rs_sf_pose_tracking_release(); }
        
        bool track(rs_sf_image& depth_frame, rs_sf_image& color_frame, const rs2_extrinsics& d2c, bool reset_request)
        {
            if (!_sp_init) {
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
            const bool track_success = rs_sf_do_scene_perception_tracking(_sdepth.data(), _scolor.data(), reset_request, depth_frame.cam_pose);
            const bool cast_success = rs_sf_do_scene_perception_ray_casting(320, 240, _sdepth.data(), _buf);
            //const bool switch_track = (track_success && cast_success) != _was_tracking;
            
            // up-sample depth
            if ((_was_tracking = (track_success && cast_success))) {
                for (int y = 0, p = 0, h = depth_frame.img_h, w = depth_frame.img_w; y < h; ++y) {
                    for (int x = 0; x < w; ++x, ++p) {
                        depth_data[p] = s_depth_data[(y * 240 / h) * 320 + (x * 320 / w)];
                    }
                }
                
                // compute color image pose
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
            return _was_tracking;
        }
        
    private:
        bool _sp_init, _was_tracking = false;
        std::unique_ptr<float[]> _buf;
        std::vector<unsigned short> _sdepth;
        std::vector<unsigned char> _scolor;
    };
}
