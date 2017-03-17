#define RS_SHAPEFIT_EXPORTS
#include "rs_shapefit.h"
#include "rs_sf_planefit.h"
#include "rs_sf_boxfit.h"

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

rs_sf_status rs_shapefit_depth_image(rs_shapefit * obj, const rs_sf_image * image, rs_sf_fit_option option)
{
    if (!obj || !image || image->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    auto pf = dynamic_cast<rs_sf_planefit*>(obj);
    if (pf) {
        if (option == RS_SHAPEFIT_OPTION_TRACK && pf->num_detected_planes() > 0)
            return pf->track_depth_image(image);   //tracking mode
        else
            return pf->process_depth_image(image); //static mode
    }

    return RS_SF_INVALID_OBJ_HANDLE;
}

rs_sf_status rs_sf_planefit_draw_planes(const rs_shapefit * obj, rs_sf_image * rgb, const rs_sf_image * src)
{
    if (!obj || !rgb || rgb->byte_per_pixel != 3) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    rs_sf_image_mono map(rgb);
    const auto status = pf->get_plane_index_map(&map, rgb->cam_pose || (src && src->cam_pose) ? 0 : -1);
    if (status == RS_SF_SUCCESS)  rs_sf_util_draw_planes(rgb, &map, src);
    return status;
}

rs_sf_status rs_sf_planefit_draw_plane_ids(const rs_shapefit * obj, rs_sf_image * mono, rs_sf_draw_opion option)
{
    if (!obj || !mono || mono->byte_per_pixel != 1) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    const auto status = pf->get_plane_index_map(mono, mono->cam_pose ? 0 : -1);
    if (status == RS_SF_SUCCESS && option == RS_SF_PLANEFIT_DRAW_SCALED)
        rs_sf_util_scale_plane_ids(mono, pf->max_detected_pid());

    return status;
}

rs_sf_status rs_sf_planefit_get_equation(const rs_shapefit * obj, int pid, float equation[4])
{
    if (!obj || !equation) return RS_SF_INVALID_ARG;
    auto pf = dynamic_cast<const rs_sf_planefit*>(obj);
    if (!pf) return RS_SF_INVALID_OBJ_HANDLE;

    return pf->get_plane_equation(pid, equation);
}

RS_SHAPEFIT_DECL rs_sf_status rs_sf_boxfit_draw_boxes(const rs_shapefit * obj, rs_sf_image * rgb, const rs_sf_image * src)
{
    if (!obj || !rgb || rgb->byte_per_pixel != 3) return RS_SF_INVALID_ARG;
    auto bf = dynamic_cast<const rs_sf_boxfit*>(obj);
    if (!bf) return RS_SF_INVALID_OBJ_HANDLE;

    if (src) rs_sf_util_convert_to_rgb_image(rgb, src);
    rs_sf_util_draw_boxes(rgb, obj->m_intrinsics, bf->get_boxes());

    return RS_SF_SUCCESS;
}