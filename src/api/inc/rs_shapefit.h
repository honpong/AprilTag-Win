#pragma once
#include "rs_shapefit_types.h"

rs_sf_planefit* rs_sf_planefit_create(const rs_sf_intrinsics* camera);
void rs_sf_planefit_delete(rs_sf_planefit* obj);