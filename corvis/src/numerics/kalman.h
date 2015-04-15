// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __KALMAN_H
#define __KALMAN_H

#include "matrix.h"

bool kalman_compute_gain(matrix &gain, const matrix &LC, const matrix &inn_cov);
void kalman_update_state(matrix &state, const matrix &gain, const matrix &inn);
void kalman_update_covariance(matrix &cov, const matrix &gain, const matrix &LC);
void kalman_update_covariance_robust(matrix &cov, const matrix &gain, const matrix &LC, const matrix inn_cov);

#endif
