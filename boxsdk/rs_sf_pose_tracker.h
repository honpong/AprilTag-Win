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
                                  int& target_width,
                                  int& target_height,
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
        camera_tracker(const rs2_intrinsics* i, const rs_sf_pose_track_resolution& resolution) :
            _sp_init(rs_sf_setup_scene_perception(i->fx, i->fy, i->ppx, i->ppy, i->width, i->height, _acc_w, _acc_h, resolution)),
            _sdepth(_acc_w * _acc_h * (_sp_init ? 1 : 0)) {}

        ~camera_tracker() { if (_sp_init) rs_sf_pose_tracking_release(); }
        
        bool track(rs_sf_image& depth_frame, rs_sf_image& color_frame, rs_sf_image& dense_frame, const rs2_extrinsics& d2c, bool reset_request)
        {
            if (!_sp_init) {
                memcpy(depth_frame.cam_pose, std::vector<float>{ 1.0f, .0f, .0f, .0f, .0f, 1.0f, .0f, .0f, .0f, .0f, 1.0f, .0f }.data(), depth_frame.cam_pose ? sizeof(float) * 12 : 0);
                depth_frame.cam_pose = nullptr;
                return true;
            }
            
            //start_time = std::chrono::steady_clock::now();
            //auto start_pose_track_time = std::chrono::steady_clock::now();
            
            auto sp_src_depth = (unsigned short*)depth_frame.data;
            auto sp_src_color = (unsigned char*)color_frame.data;
            auto sp_dst_depth = (unsigned short*)dense_frame.data;
            
            // do pose tracking
            const bool track_success = rs_sf_do_scene_perception_tracking(sp_src_depth, sp_src_color, reset_request, depth_frame.cam_pose);
            const bool cast_success = rs_sf_do_scene_perception_ray_casting(_acc_w, _acc_h, sp_dst_depth, _buf);
            //const bool switch_track = (track_success && cast_success) != _was_tracking;
            
            // up-sample depth
            if ((_was_tracking = (track_success && cast_success)))
            {
                // compute color image pose
                auto* d = depth_frame.cam_pose, *c = color_frame.cam_pose;
                transform(c, d, d2c.rotation, d2c.translation);
            }

            return _was_tracking;
        }
        
    private:

        int _acc_w = 0, _acc_h = 0;
        bool _sp_init, _was_tracking = false;
        std::unique_ptr<float[]> _buf;
        std::vector<unsigned short> _sdepth;

        // dst = r * src + t
        static void transform(float dst[12], const float src[12], const float r[9], const float t[3])
        {
            // compute dst image pose
            dst[0] = r[0] * src[0] + r[1] * src[4] + r[2] * src[8];
            dst[1] = r[0] * src[1] + r[1] * src[5] + r[2] * src[9];
            dst[2] = r[0] * src[2] + r[1] * src[6] + r[2] * src[10];
            dst[3] = r[0] * src[3] + r[1] * src[7] + r[2] * src[11] + t[0];
            dst[4] = r[3] * src[0] + r[4] * src[4] + r[5] * src[8];
            dst[5] = r[3] * src[1] + r[4] * src[5] + r[5] * src[9];
            dst[6] = r[3] * src[2] + r[4] * src[6] + r[5] * src[10];
            dst[7] = r[3] * src[3] + r[4] * src[7] + r[5] * src[11] + t[1];
            dst[8] = r[6] * src[0] + r[7] * src[4] + r[8] * src[8];
            dst[9] = r[6] * src[1] + r[7] * src[5] + r[8] * src[9];
            dst[10] = r[6] * src[2] + r[7] * src[6] + r[8] * src[10];
            dst[11] = r[6] * src[3] + r[7] * src[7] + r[8] * src[11] + t[2];
        }
    };
}
