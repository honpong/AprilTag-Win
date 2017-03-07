#define RS_SHAPEFIT_EXPORTS
#include "rs_shapefit.h"
#include "rs_sf_planefit.h"

rs_sf_planefit * rs_sf_planefit_create(const rs_sf_intrinsics * camera)
{
    return new rs_sf_planefit(camera);
}

void rs_sf_planefit_delete(rs_sf_planefit * obj)
{
    if (obj) delete obj;
}

rs_sf_status rs_sf_planefit_depth_image(rs_sf_planefit * obj, const rs_sf_image * image, rs_sf_planefit_option option)
{
    if (!obj || !image || image->byte_per_pixel != 2) return RS_SF_INVALID_ARG;

    if (option == RS_SF_PLANEFIT_OPTION_TRACK && obj->num_detected_planes() > 0)
        return obj->track_depth_image(image);   //tracking mode
    else
        return obj->process_depth_image(image); //static mode
}

rs_sf_status rs_sf_planefit_draw_planes(const rs_sf_planefit * obj, rs_sf_image * rgb, const rs_sf_image * src)
{
    if (!obj || !rgb || rgb->byte_per_pixel != 3) return RS_SF_INVALID_ARG;

    rs_sf_image_mono map(rgb);
    const auto status = obj->get_plane_index_map(&map, rgb->cam_pose || (src && src->cam_pose) ? 0 : -1);
    if (status == RS_SF_SUCCESS)  draw_planes(rgb, &map, src);
    return status;
}

RS_SHAPEFIT_DECL rs_sf_status rs_sf_planefit_draw_plane_ids(const rs_sf_planefit * obj, rs_sf_image * mono, rs_sf_planefit_draw_opion option)
{
    if (!obj || !mono || mono->byte_per_pixel != 1) return RS_SF_INVALID_ARG;

    const auto status = obj->get_plane_index_map(mono, mono->cam_pose ? 0 : -1);
    if (status == RS_SF_SUCCESS && option == RS_SF_PLANEFIT_DRAW_SCALED)
        scale_plane_ids(mono, obj->max_detected_pid());

    return status;
}
