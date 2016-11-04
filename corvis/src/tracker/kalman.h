// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#ifndef __KALMAN_H
#define __KALMAN_H

#include "matrix.h"

bool kalman_compute_gain(matrix &gain, const matrix &LC, const matrix &inn_cov, matrix &tmp);
void kalman_update_state(matrix &state, const matrix &gain, const matrix &inn);
void kalman_update_covariance(matrix &cov, const matrix &gain, const matrix &LC);
void kalman_update_covariance_robust(matrix &cov, const matrix &gain, const matrix &LC, const matrix inn_cov);

#endif
