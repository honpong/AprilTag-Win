// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#ifndef __KALMAN_H
#define __KALMAN_H

#include "matrix.h"

bool kalman_compute_gain(matrix &K, const matrix &HP, matrix &S);
void kalman_update_state(matrix &state, const matrix &K, const matrix &inn);
void kalman_update_covariance(matrix &P, const matrix &K, const matrix &HP);

#endif
