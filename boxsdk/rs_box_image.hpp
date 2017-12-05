/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs_box_image.hpp
//  boxsdk
//
//  Created by Hon Pong (Gary) Ho
//
#pragma once
#ifndef rs_box_image_hpp
#define rs_box_image_hpp

#include <cfloat>
#include "rs_shapefit.h"

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
    
    static void get_inverse_pose(const float pose[12], float inv_pose[12])
    {
        inv_pose[0] = pose[0]; inv_pose[1] = pose[4]; inv_pose[2] = pose[8];
        inv_pose[3] = -((pose[0] * pose[3]) + (pose[4] * pose[7]) + (pose[8] * pose[11]));
        inv_pose[4] = pose[1]; inv_pose[5] = pose[5]; inv_pose[6] = pose[9];
        inv_pose[7] = -((pose[1] * pose[3]) + (pose[5] * pose[7]) + (pose[9] * pose[11]));
        inv_pose[8] = pose[2]; inv_pose[9] = pose[6]; inv_pose[10] = pose[10];
        inv_pose[11] = -((pose[2] * pose[3]) + (pose[6] * pose[7]) + (pose[10] * pose[11]));
    }
    
    static void project_box_onto_frame(const rs2_measure_box& box, const rs2_measure_camera_state& f, rs2_measure_box_wireframe& box_frame)
    {
        static const float line_idx[][2][3] = {
            { { -.5f,-.5f,-.5f },{ .5f,-.5f,-.5f } },{ { -.5f,-.5f,.5f },{ .5f,-.5f,.5f } },{ { -.5f,.5f,-.5f },{ .5f,.5f,-.5f } },{ { -.5f,.5f,.5f },{ .5f,.5f,.5f } },
            { { -.5f,-.5f,-.5f },{ -.5f,.5f,-.5f } },{ { -.5f,-.5f,.5f },{ -.5f,.5f,.5f } },{ { .5f,-.5f,-.5f },{ .5f,.5f,-.5f } },{ { .5f,-.5f,.5f },{ .5f,.5f,.5f } },
            { { -.5f,-.5f,-.5f },{ -.5f,-.5f,.5f } },{ { -.5f,.5f,-.5f },{ -.5f,.5f,.5f } },{ { .5f,-.5f,-.5f },{ .5f,-.5f,.5f } },{ { .5f,.5f,-.5f },{ .5f,.5f,.5f } } };
        
        float pt[3], inv_pose[12];
        get_inverse_pose(f.camera_pose, inv_pose);
        for (int l = 0; l < 12; ++l) {
            for (int i = 0; i < 2; ++i) {
                for (int d = 0; d < 3; ++d)
                    pt[d] = box.center[d] + box.axis[0][d] * line_idx[l][i][0] + box.axis[1][d] * line_idx[l][i][1] + box.axis[2][d] * line_idx[l][i][2];
                const auto x = pt[0] * inv_pose[0] + pt[1] * inv_pose[1] + pt[2] * inv_pose[2] + inv_pose[3];
                const auto y = pt[0] * inv_pose[4] + pt[1] * inv_pose[5] + pt[2] * inv_pose[6] + inv_pose[7];
                const auto z = pt[0] * inv_pose[8] + pt[1] * inv_pose[9] + pt[2] * inv_pose[10] + inv_pose[11];
                const auto iz = fabs(z) <= FLT_EPSILON ? 0 : 1.0f / z;
                box_frame.end_pt[l][i][0] = x * f.intrinsics->fx * iz + f.intrinsics->ppx;
                box_frame.end_pt[l][i][1] = y * f.intrinsics->fy * iz + f.intrinsics->ppy;
            }
        }
    }
}
#endif /* rs_box_image_hpp */
