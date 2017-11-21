/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include <memory>

enum rs_sf_pose_track_resolution : int
{
    RS_SF_LOW_RESOLUTION = 0,
    RS_SF_MED_RESOLUTION = 1,
    RS_SF_HIGH_RESOLUTION = 2
};

bool rs_sf_setup_scene_perception(
    float rfx, float rfy, float rpx, float rpy, unsigned int rw, unsigned int rh,
    int target_width,
    int target_height,
    rs_sf_pose_track_resolution resolution);

bool rs_sf_do_scene_perception_tracking(
    unsigned short* depth_data,
    unsigned char* rgb_data,
    bool& reset_request,
    float cam_pose[12] = nullptr);

bool rs_sf_do_scene_perception_ray_casting(
    int image_width,
    int image_height,
    unsigned short* dst_depth_data,
    std::unique_ptr<float[]>& buffer);

void rs_sf_pose_tracking_release();
