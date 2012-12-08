// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __KALMAN_H
#define __KALMAN_H

#include "matrix.h"

void block_update(matrix &c, const matrix &lu);
void time_update(matrix &c, const matrix &ltu_, const matrix &p_cov, const f_t dt);
void meas_update(matrix &state, matrix &cov, const matrix &innov, const matrix &lp, const matrix &m_cov);

#endif
