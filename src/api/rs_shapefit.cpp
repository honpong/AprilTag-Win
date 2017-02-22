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
    if (!obj || !image) return RS_SF_INVALID_ARG;

    if (option == RS_SF_PLANEFIT_OPTION_TRACK && obj->num_detected_planes() > 0)
        return obj->track_depth_image(image);   //tracking mode
    else
        return obj->process_depth_image(image); //static mode
}