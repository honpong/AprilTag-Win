#include "rs_sf_planefit.h"

rs_sf_planefit::rs_sf_planefit(const rs_sf_intrinsics * camera)
{
    m_intrinsics = *camera;
}
