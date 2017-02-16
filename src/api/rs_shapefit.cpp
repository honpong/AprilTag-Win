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
