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
}
#endif /* rs_box_image_hpp */