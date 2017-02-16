#pragma once
#include "rs_shapefit_types.h"

struct rs_sf_planefit
{
    rs_sf_planefit(const rs_sf_intrinsics* camera);
    ~rs_sf_planefit(){}

    rs_sf_intrinsics m_intrinsics;

};