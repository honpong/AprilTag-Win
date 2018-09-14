/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2017 Intel Corporation. All Rights Reserved.

*******************************************************************************/
//
//  rs_shapefit.cpp
//  algo-core
//
//  Created by Hon Pong (Gary) Ho
//
#define RS_SHAPEFIT_EXPORTS
#include "rs_shapefit.h"
#include "rs_sf_planefit.hpp"
#include "rs_sf_boxfit.hpp"
#include "rs_sf_boxfit_color.hpp"

rs_shapefit * rs_shapefit_create(const rs_sf_intrinsics * camera, rs_shapefit_capability capability)
{
    switch (capability) {
    case RS_SHAPEFIT_PLANE: return new rs_sf_planefit(camera);
    case RS_SHAPEFIT_BOX: return new rs_sf_boxfit(camera);
    case RS_SHAPEFIT_BOX_COLOR: return new rs_sf_boxfit_color(camera);
    default: return nullptr;
    }
}

void rs_shapefit_delete(rs_shapefit * obj)
{
    if (obj) delete obj;
}

rs_sf_status rs_shapefit_depth_image(rs_shapefit * obj, const rs_sf_image * depth)
{
    if (!obj || !depth || depth->byte_per_pixel != 2) return RS_SF_INVALID_ARG;
    auto* pf = dynamic_cast<rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    if (!pf->m_input_mutex.try_lock()) return RS_SF_BUSY;
    auto new_input_status = pf->set_locked_inputs(depth); 
    if (new_input_status == RS_SF_SUCCESS) {
        pf->m_task_status = std::async(std::launch::async, [pf = pf]() {
            if (pf->get_option_track() == rs_shapefit::CONTINUE &&
                pf->num_detected_planes() > 0)
                return pf->track_depth_image();   //tracking mode
            else
                return pf->process_depth_image(); //single frame mode
        });
    }
    pf->run_task(pf->get_option_async_process_wait());
    pf->m_input_mutex.unlock();
    return new_input_status;
}

rs_sf_status rs_shapefit_set_option(rs_shapefit * obj, rs_sf_fit_option option, double value)
{
    if (!obj) return RS_SF_INVALID_ARG;
    if (option < 0 || option >= RS_SF_OPTION_COUNT) return RS_SF_INDEX_OUT_OF_BOUND;
    return obj->set_option(option, value);
}

rs_sf_status rs_sf_planefit_get_plane_ids(const rs_shapefit * obj, rs_sf_image * id_map)
{
    if (!obj || !id_map || id_map->byte_per_pixel != 1) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    const auto status = pf->get_plane_index_map(id_map);
    if (status == RS_SF_SUCCESS) {
        switch (pf->get_option_get_plane_id()) {
        case rs_shapefit::SCALED: rs_sf_util_scale_plane_ids(id_map, pf->max_detected_pid()); break;
        case rs_shapefit::REMAP: rs_sf_util_remap_plane_ids(id_map); break;
            default: break;
        }
    }
    return status;
}

rs_sf_status rs_sf_planefit_get_planes(const rs_shapefit * obj, rs_sf_image * rgb, rs_sf_plane planes[RS_SF_MAX_PLANE_COUNT], float * point_buffer)
{
    if (!obj || (!planes && !rgb)) return RS_SF_INVALID_ARG;
    if (rgb && rgb->byte_per_pixel != 3) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    rs_sf_plane tmp[RS_SF_MAX_PLANE_COUNT];
    std::vector<float> buf;
    if (rgb &&  point_buffer == nullptr) {
        buf.resize(rgb->num_pixel() * 3);
        point_buffer = buf.data();
    }
    if (planes == nullptr) { planes = tmp; }

    // call internal get plane api
    auto status = pf->get_planes(planes, point_buffer);

    // call internal draw plane api
    if (rgb != nullptr && status >= RS_SF_SUCCESS)
        rs_sf_util_draw_plane_contours(rgb, pose_t().set_pose(rgb->cam_pose),
            rs_sf_util_match_intrinsics(rgb,pf->m_intrinsics), planes, 15);

    return status;
}

rs_sf_status rs_sf_planefit_draw_planes(const rs_shapefit * obj, rs_sf_image * rgb, const rs_sf_image * bkg)
{
    if (!obj || !rgb || rgb->byte_per_pixel != 3) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    rs_sf_image_mono map(rgb);
    const auto status = pf->get_plane_index_map(&map);
    if (status == RS_SF_SUCCESS) {
        if (bkg) { rs_sf_util_convert_to_rgb_image(rgb, bkg); }
        rs_sf_util_draw_plane_ids(rgb, &map, bkg == nullptr);
    }
    return status;
}

rs_sf_status rs_sf_boxfit_get_box(const rs_shapefit * obj, int box_id, rs_sf_box * box, float *maturity)
{
    if (!obj || !box) return RS_SF_INVALID_ARG;
    auto bf = dynamic_cast<const rs_sf_boxfit*>(obj);
    if (!bf) return RS_SF_INVALID_OBJ_HANDLE;

    if (0 < box_id || box_id >= bf->num_detected_boxes()) return RS_SF_INDEX_OUT_OF_BOUND;
    float history_progress;
    *box = bf->get_box(box_id, history_progress);
    if (maturity) { *maturity = history_progress; }
#ifdef _DEBUG
    return history_progress >= 0.0f ? RS_SF_SUCCESS : RS_SF_ITEM_NOT_READY;
#else 
    return history_progress >= 0.5f ? RS_SF_SUCCESS : RS_SF_ITEM_NOT_READY;
#endif
}

rs_sf_status rs_sf_boxfit_draw_boxes(const rs_shapefit * obj, rs_sf_image * rgb, const rs_sf_image * bkg, const unsigned char color[3])
{
    if (!obj || !rgb || rgb->byte_per_pixel != 3) return RS_SF_INVALID_ARG;
    auto bf = dynamic_cast<const rs_sf_boxfit*>(obj);
    if (!bf) return RS_SF_INVALID_OBJ_HANDLE;

    if (bkg) { rs_sf_util_convert_to_rgb_image(rgb, bkg); }
    pose_t pose; pose.set_pose(rgb->cam_pose || !bkg ? rgb->cam_pose : bkg->cam_pose);
    rs_sf_util_draw_boxes(rgb, pose, rs_sf_util_match_intrinsics(rgb, obj->m_intrinsics), bf->get_boxes(),
        color ? b3{ color[0],color[1],color[2] } : b3{ 255, 255, 0 });

    return RS_SF_SUCCESS;
}

rs_sf_status rs_sf_boxfit_raycast_boxes(const rs_shapefit * obj, const rs_sf_box* boxes, int num_box, rs_sf_image* depth, const rs_sf_image* init)
{
    if (!obj || !depth || depth->byte_per_pixel != 2) return RS_SF_INVALID_ARG;
    auto bf = dynamic_cast<const rs_sf_boxfit*>(obj);
    if (!boxes && !bf) return RS_SF_INVALID_OBJ_HANDLE;

    if (!init) { rs_sf_util_set_to_zeros(depth); }
    else if (init != depth) {
        rs_sf_memcpy(depth->data, init->data, std::min(depth->num_char(), init->num_char()));
    }

    pose_t pose; pose.set_pose(depth->cam_pose);
    auto bs = (boxes ? std::vector<rs_sf_box>(boxes, boxes + num_box) : bf->get_boxes());
    rs_sf_util_raycast_boxes(depth, pose, (float)obj->m_param[RS_SF_OPTION_DEPTH_UNIT], rs_sf_util_match_intrinsics(depth, obj->m_intrinsics), bs);
    
    return RS_SF_SUCCESS;
}
