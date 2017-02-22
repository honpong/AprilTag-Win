#pragma once
#ifndef rs_sf_util_h
#define rs_sf_util_h

#include "rs_shapefit.h"
#include <Eigen/Dense>
#include <memory>

void set_to_zeros(rs_sf_image* img);
void eigen_3x3_real_symmetric(float D[6], float u[3], float v[3][3]);

#endif // ! rs_sf_util_h

