/*******************************************************************************
 
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2018 Intel Corporation. All Rights Reserved.
 
 *******************************************************************************/
//
//  rs_sf_boxfit_color.hpp
//  algo-core
//
//  Created by Hon Pong (Gary) Ho
//
#pragma once
#ifndef rs_sf_boxfit_color_hpp
#define rs_sf_boxfit_color_hpp

#include "rs_sf_boxfit.hpp"


struct rs_sf_boxfit_color : public rs_sf_boxfit
{
    struct parameter
    {
        float max_adjust_box_in_pixel = 20.0f; //max box dimension adjust in pixel unit
        float max_adjust_box_percent  = 0.15f; //max box dimension adjust in percentage
        float edge_fitting_step       = 0.5f;
        int dx = 4;
        int dy = 4;
    };

    rs_sf_boxfit_color(const rs_sf_intrinsics camera[2]);
    ~rs_sf_boxfit_color() override { run_task(-1); }
    rs_sf_status set_option(rs_sf_fit_option option, double value) override;
    rs_sf_status set_locked_inputs(const rs_sf_image* img) override;
    
protected:
    
    parameter m_param;
    rs_sf_intrinsics m_intrinsics_color;
    rs_sf_intrinsics* m_camera[2];
    
    rs_sf_status refit_boxes(box_scene& view) override;
    
    struct box_candidate : public box_record
    {
        int a, s;
        float pn, da;
        box_candidate(const box_record& ref, int _a=-1, int _sign=-1, float _pn=0.0f, float signed_adjust_in_meter=0.0f) : box_record(ref), a(_a), s(_sign), pn(_pn), da(signed_adjust_in_meter) {}
    };
    
    struct box_color_scene
    {
        box_scene* parent;
        pose_t cam_pose;
        int dx, dy;
        std::shared_ptr<rs_sf_image_rgb> debug;
        unsigned char& debug_pixel(int i, int j, int c) const { return debug->data[(j*debug->img_w+i)*debug->byte_per_pixel+c];};
        
        void reset(box_scene& ref, rs_sf_boxfit_color& bf);
        
        std::function<v3(const v3&)>   world_to_camera;
        std::function<v3(const v3&)>   world_to_camera_axis;
        std::function<v2(const v3&)>   camera_to_image;
        std::function<v3(const v2&)>   image_to_normalized_camera;
        std::function<bool(const v2&)> in_fov;
        std::function<unsigned char(int,int,int)> pixel;
        v2 world_to_image(const v3& ptw) const { return camera_to_image(world_to_camera(ptw)); }

        float gradient(int i, int j) const;
        float score_pixel(float u, float v) const;
        float score_line(const v2& p0, const v2& p1) const;
        float score_box(const box_candidate& box) const;
        
    } m_box_color_scene;

private:
    box_candidate generate_box(const box_record& ref, int axis, int sign, float pn) const;
    box_record make_ref(const box& ref) const {
        return box_record(ref, m_box_color_scene.cam_pose, m_intrinsics_color, rs_sf_boxfit::m_param.fov_margin);
    }
    box combine(box ref, const float best_da[3][2]) const;
    box_record adjust_box(box ref, int axis, int sign, float signed_adjust_in_meter) const;
};

#endif // ! rs_sf_boxfit_color_hpp

