/*******************************************************************************
*
* Perceptual Computing, Intel Corporation
* Created by: Hon Pong (Gary) Ho
*
*******************************************************************************/
#define RS_SHAPEFIT_EXPORTS
#include "rs_shapefit.h"
#include "rs_sf_planefit.hpp"
#include "rs_sf_boxfit.hpp"

rs_shapefit * rs_shapefit_create(const rs_sf_intrinsics * camera, rs_shapefit_option option)
{
    switch (option) {
    case RS_SHAPEFIT_PLANE: return new rs_sf_planefit(camera);
    case RS_SHAPEFIT_BOX: return new rs_sf_boxfit(camera);
    default: return nullptr;
    }
}

void rs_shapefit_delete(rs_shapefit * obj)
{
    if (obj) delete obj;
}

RS_SHAPEFIT_DECL rs_sf_status rs_shapefit_set_option(rs_shapefit * obj, rs_sf_fit_option option, double value)
{
    if (!obj) return RS_SF_INVALID_ARG;
    if (option < 0 || option >= RS_SF_OPTION_COUNT) return RS_SF_INDEX_OUT_OF_BOUND;
    return obj->set_option(option, value);
}

rs_sf_status rs_shapefit_depth_image(rs_shapefit * obj, const rs_sf_image * image)
{
    if (!obj || !image || image->byte_per_pixel != 2) return RS_SF_INVALID_ARG;
    auto* pf = dynamic_cast<rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    if (!pf->m_input_mutex.try_lock()) return RS_SF_BUSY;
    auto new_input_status = pf->set_locked_inputs(image); 
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

rs_sf_status rs_sf_planefit_draw_planes(const rs_shapefit * obj, rs_sf_image * rgb, const rs_sf_image * src)
{
    if (!obj || !rgb || rgb->byte_per_pixel != 3) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    rs_sf_image_mono map(rgb);
    const auto status = pf->get_plane_index_map(&map);
    if (status == RS_SF_SUCCESS) {
        if (src) { rs_sf_util_convert_to_rgb_image(rgb, src); }
        rs_sf_util_draw_plane_ids(rgb, &map, src == nullptr);
    }
    return status;
}

rs_sf_status rs_sf_planefit_get_plane_ids(const rs_shapefit * obj, rs_sf_image * mono)
{
    if (!obj || !mono || mono->byte_per_pixel != 1) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    const auto status = pf->get_plane_index_map(mono);
    if (status == RS_SF_SUCCESS) {
        switch (pf->get_option_get_plane_id()) {
        case rs_shapefit::SCALED: rs_sf_util_scale_plane_ids(mono, pf->max_detected_pid()); break;
        case rs_shapefit::REMAP: rs_sf_util_remap_plane_ids(mono); break;
        }
    }
    return status;
}

RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_get_planes(const rs_shapefit * obj, rs_sf_image * img, rs_sf_plane planes[255], float * point_buffer)
{
    if (!obj || (!planes && !img)) return RS_SF_INVALID_ARG;
    if (img && img->byte_per_pixel != 3) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    rs_sf_plane tmp[MAX_VALID_PID + 2];
    std::vector<float> buf;
    if (img &&  point_buffer == nullptr) {
        buf.resize(img->num_pixel() * 3);
        point_buffer = buf.data();
    }
    if (planes == nullptr) { planes = tmp; }

    // call internal get plane api
    auto status = pf->get_planes(planes, point_buffer);

    // call internal draw plane api
    if (img != nullptr && status >= RS_SF_SUCCESS)   
        rs_sf_util_draw_plane_contours(img, pose_t().set_pose(img->cam_pose), 
            img->intrinsics ? *img->intrinsics : pf->m_intrinsics, planes, 25);

    return status;
}

RS_SHAPEFIT_DECL rs_sf_status rs_sf_boxfit_get_box(const rs_shapefit * obj, int box_id, rs_sf_box * dst)
{
    if (!obj || !dst) return RS_SF_INVALID_ARG;
    auto bf = dynamic_cast<const rs_sf_boxfit*>(obj);
    if (!bf) return RS_SF_INVALID_OBJ_HANDLE;

    if (0 < box_id || box_id >= bf->num_detected_boxes()) return RS_SF_INDEX_OUT_OF_BOUND;
    *dst = bf->get_box(box_id);
    return RS_SF_SUCCESS;
}

RS_SHAPEFIT_DECL rs_sf_status rs_sf_boxfit_draw_boxes(const rs_shapefit * obj, rs_sf_image * rgb, const rs_sf_image * src, const unsigned char c[3])
{
    if (!obj || !rgb || rgb->byte_per_pixel != 3) return RS_SF_INVALID_ARG;
    auto bf = dynamic_cast<const rs_sf_boxfit*>(obj);
    if (!bf) return RS_SF_INVALID_OBJ_HANDLE;

    if (src) { rs_sf_util_convert_to_rgb_image(rgb, src); }
    pose_t pose; pose.set_pose(rgb->cam_pose || !src ? rgb->cam_pose : src->cam_pose);
    rs_sf_util_draw_boxes(rgb, pose, 
        rgb->intrinsics? *rgb->intrinsics : obj->m_intrinsics, bf->get_boxes(), 
        c ? b3{ c[0],c[1],c[2] } : b3{255, 255, 0});

    return RS_SF_SUCCESS;
}
