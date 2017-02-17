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

rs_sf_status rs_sf_planefit_depth_image(rs_sf_planefit* obj, const rs_sf_image * img)
{
    return obj->process_depth_image(img);
}
