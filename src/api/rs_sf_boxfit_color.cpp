/*******************************************************************************
 
 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2018 Intel Corporation. All Rights Reserved.
 
 *******************************************************************************/
//
//  rs_sf_boxfit_color.cpp
//  algo-core
//
//  Created by Hon Pong (Gary) Ho
//
#include "rs_sf_boxfit_color.hpp"

rs_sf_boxfit_color::rs_sf_boxfit_color(const rs_sf_intrinsics camera[2]) : rs_sf_boxfit(camera)
{
    m_intrinsics_color = camera[1];
    m_camera[0] = &m_intrinsics;
    m_camera[1] = &m_intrinsics_color;
    
    m_view.src_color_img = std::make_unique<rs_sf_image_rgb>(src_w(),src_h());
    m_ref_view.src_color_img = std::make_unique<rs_sf_image_rgb>(src_w(), src_h());
    m_view.src_color_img->intrinsics = m_ref_view.src_color_img->intrinsics = m_camera[1];
}

rs_sf_status rs_sf_boxfit_color::set_option(rs_sf_fit_option option, double value)
{
    return rs_sf_boxfit::set_option(option, value);
}

rs_sf_status rs_sf_boxfit_color::set_locked_inputs(const rs_sf_image* img)
{
    const auto status = rs_sf_boxfit::set_locked_inputs(img);
    if (status == RS_SF_SUCCESS) rs_sf_util_copy_color_image(*m_view.src_color_img, &img[1]);
    return status;
}

rs_sf_boxfit::box rs_sf_boxfit_color::combine(box ref, const float best_da[3][2]) const
{
    ref.center +=
    (ref.axis.col(0) * (best_da[0][0] - best_da[0][1]) * 0.5f +
     ref.axis.col(1) * (best_da[1][0] - best_da[1][1]) * 0.5f +
     ref.axis.col(2) * (best_da[2][0] - best_da[2][1]) * 0.5f);
    ref.dimension[0] += (best_da[0][0] + best_da[0][1]);
    ref.dimension[1] += (best_da[1][0] + best_da[1][1]);
    ref.dimension[2] += (best_da[2][0] + best_da[2][1]);
    return ref;
}

rs_sf_status rs_sf_boxfit_color::refit_boxes(box_scene& view)
{
    m_box_color_scene.reset(view, *this);
#ifndef NDEBUG
    auto& debug = m_box_color_scene.debug = std::make_shared<rs_sf_image_rgb>(view.plane_scene->src_color_img.get());
#endif

    for(auto& pair : view.plane_pairs)
    {
        if(!pair.new_box) continue;

        for(auto axis : {0,1,2}){
            const auto box_original = make_ref(*pair.new_box);
            const float MAX_PN = m_param.max_adjust_box_in_pixel;
            const float MAX_DA = box_original.dimension[axis] * m_param.max_adjust_box_percent;
            float best_da[3][2] = {};
            for(auto sign : {0,1}){
                float max_score = 0.0f;
                for(float pn=-MAX_PN;pn<=MAX_PN;pn+=m_param.edge_fitting_step){
                    box_candidate box_c = generate_box(box_original, axis, sign, pn);
                    if(std::abs(box_c.da)<MAX_DA){
                        float bscore = m_box_color_scene.score_box(box_c);
                        if(bscore > max_score){
                            max_score = bscore;
                            best_da[axis][sign] = box_c.da;
                        }
                    }
                }
            }
            *pair.new_box = combine(box_original,best_da);
            
#ifndef NDEBUG
#if OPENCV_FOUND
            static const b3 c[] = {b3(255,0,0),b3(0,255,0),b3(0,0,255)};
            rs_sf_util_draw_boxes(debug.get(), m_box_color_scene.cam_pose, m_intrinsics_color, {pair.new_box->to_rs_sf_box()}, c[axis]);
            cv::Mat color = cv::Mat(debug->img_h, debug->img_w, CV_8UC3, debug->data).clone();
            //cv::imshow("generate_adjusted_box",color);
            //static int i=0;
            //cv::imwrite("/Users/hho11/Desktop/temp/debug/debug"+std::to_string(i++)+".png",color);
#endif
#endif
        }
    }
    return RS_SF_SUCCESS;
}

rs_sf_boxfit_color::box_candidate rs_sf_boxfit_color::generate_box(const box_record& ref, int axis, int sign, float pn) const
{
    // direction in world 3d
    const v3 dir = ref.axis.col(axis) * (1-sign*2);
    
    float min_da = m_param.max_adjust_box_in_pixel;
    bool adjustable = false;
    for(auto l : {0,1,2,3}){
        if(ref.is_visible_line(axis, sign, l)){
            
            const v3 midpt_l0 = (ref.pt[axis][sign][l]+ref.pt[axis][sign][(l+1)%4])*0.5f;
            const v2 imgpt_l0 = m_box_color_scene.world_to_image(midpt_l0);
            const v2 imgpt_l1 = m_box_color_scene.world_to_image(midpt_l0 + dir);
            const v2 imgpt_ln = imgpt_l0 + (imgpt_l1 - imgpt_l0).normalized() * pn;
            
            //new mid point that is px pixels away from midpt_l0 along direction signed axis[a]
            const v3    pdz_c = m_box_color_scene.image_to_normalized_camera(imgpt_ln);
            const v3    nor_c = m_box_color_scene.world_to_camera_axis(ref.axis.col((axis+1)%3));
            const v3    mid_c = m_box_color_scene.world_to_camera(midpt_l0);
            const float d     = -nor_c.dot(mid_c);
            const float div   = pdz_c.dot(nor_c);
            const float z_c   = (std::abs(div)>FLT_EPSILON? -d/div : 0.0f);
            const float da    = z_c!=0.0f?(pdz_c*z_c-mid_c).norm() : 0.0f;
            
            min_da = std::min(min_da, da);
            adjustable = true;
        }
    }
    
    const float da = adjustable ? (pn>=0?min_da:-min_da) : 0.0f;
    return box_candidate(adjust_box(ref,axis,sign,da),axis,sign,pn,da);
}

rs_sf_boxfit::box_record rs_sf_boxfit_color::adjust_box(box ref, int axis, int sign, float signed_adjust_in_meter) const
{
    ref.center += ref.axis.col(axis) * (0.5f-sign) * signed_adjust_in_meter;
    ref.dimension[axis] += signed_adjust_in_meter;
    return make_ref(ref);
}

float rs_sf_boxfit_color::box_color_scene::gradient(int i, int j) const
{
    return
    std::log(std::abs(pixel(i+dx,j,0) - pixel(i-dx,j,0)) +
             std::abs(pixel(i,j+dy,0) - pixel(i,j-dy,0))+1)+
    std::log(std::abs(pixel(i+dx,j,1) - pixel(i-dx,j,1)) +
             std::abs(pixel(i,j+dy,1) - pixel(i,j-dy,1))+1)+
    std::log(std::abs(pixel(i+dx,j,2) - pixel(i-dx,j,2)) +
             std::abs(pixel(i,j+dy,2) - pixel(i,j-dy,2))+1);
}

float rs_sf_boxfit_color::box_color_scene::score_pixel(float u, float v) const
{
    const int us = (int)std::floor(u), ul = (int)std::ceil(u);
    const int vs = (int)std::floor(v), vl = (int)std::ceil(v);
    return
    (ul - u) * (vl - v) * gradient(us,vs) +
    (ul - u) * (v - vs) * gradient(us,vl) +
    (u - us) * (vl - v) * gradient(ul,vs) +
    (u - us) * (v - vs) * gradient(ul,vl);
}

float rs_sf_boxfit_color::box_color_scene::score_line(const v2& p0, const v2& p1) const
{
    if(!in_fov(p0)||!in_fov(p1)) return -0.0f;
    
    const int   edge_pt_count = NUM_BOX_PLANE_BIN;//std::max(1,(int)std::ceil(edge_length));
    const float edge_delta  = 1.0f / edge_pt_count;
    float edge_score = 0.0f;
    for(int dl=0; dl <= edge_pt_count; ++dl){
        v2 pt = p0 * (1.0f - dl * edge_delta) + p1 * dl * edge_delta;
        edge_score += score_pixel(pt.x(), pt.y());
#ifndef NDEBUG
        debug_pixel(pt.x(),pt.y(),0) = debug_pixel(pt.x(),pt.y(),1) =
        debug_pixel(pt.x(),pt.y(),2) = 0;
#endif
    }
        
    return edge_score / edge_pt_count;
}

void rs_sf_boxfit_color::box_color_scene::reset(box_scene& ref, rs_sf_boxfit_color& host)
{
    parent = &ref;
    dx  = host.m_param.dx; dy = host.m_param.dy;
    auto rgb = ref.plane_scene->src_color_img.get();
    auto cam_pose_inv = cam_pose.set_pose(rgb->cam_pose).invert();
    auto margin = std::max((float)std::max(dx,dy),host.rs_sf_boxfit::m_param.fov_margin);

    pixel = [data=rgb->data,img_w=rgb->img_w,bpc=rgb->byte_per_pixel](int i, int j, int c){
        return data[(j*img_w+i)*bpc+c];
    };
    world_to_camera = [cam_pose_inv=cam_pose_inv](const v3& ptw){
        return cam_pose_inv.transform(ptw);
    };
    world_to_camera_axis = [rotation=cam_pose_inv.rotation](const v3& axis) { return rotation*axis; };
    camera_to_image = [fx=rgb->intrinsics->fx, fy=rgb->intrinsics->fy, ppx=rgb->intrinsics->ppx, ppy=rgb->intrinsics->ppy](const v3& ptc){
        return v2(ptc.x() * fx / ptc.z() + ppx, ptc.y() * fy / ptc.z() + ppy);
    };
    image_to_normalized_camera = [fx=rgb->intrinsics->fx, fy=rgb->intrinsics->fy, ppx=rgb->intrinsics->ppx, ppy=rgb->intrinsics->ppy](const v2& pt){
        return v3((pt.x()-ppx)/fx, (pt.y()-ppy)/fy, 1);
    };
    in_fov = [sx=margin,ex=(rgb->img_w-margin),sy=margin,ey=(rgb->img_h-margin)](const v2& pt){
        return sx<=pt.x() && std::ceil(pt.x())<ex && sy<=pt.y() && std::ceil(pt.y())<ey;
    };
}

float rs_sf_boxfit_color::box_color_scene::score_box(const box_candidate& box) const
{
    float score_sum = 0.0f;
    int num_vis_ln = 0;
    for (auto l : {0,1,2,3}){
        if (box.is_visible_line(box.a,box.s,l)){
            const v2 p0 = world_to_image(box.pt[box.a][box.s][l]);
            const v2 p1 = world_to_image(box.pt[box.a][box.s][(l+1)%4]);
            score_sum += score_line(p0, p1);
            num_vis_ln++;
        }
    }
    return score_sum/std::max(1,num_vis_ln);
}
