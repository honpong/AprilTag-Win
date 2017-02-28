// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#include <assert.h>
#include "kalman.h"
#include "matrix.h"
#include <string.h>

bool kalman_compute_gain(matrix &K, const matrix &HP, matrix &S)
{
    //K = P * H' * inv(S)
    //Lapack uses column-major ordering, so we feed it PH' and get K directly, in the form X * A = B' rather than A' * X = B
    K.resize(HP.cols(), HP.rows());
    K.map() = HP.map().transpose();
    return matrix_solve(S, K); // K = K inv(S), S is destroyed
}

void kalman_update_state(matrix &state, const matrix &K, const matrix &inn)
{
    //state.T += innov.T * K.T
    matrix_product(state, inn, K, false, true, 1.0);
}

void kalman_update_covariance(matrix &P, const matrix &K, const matrix &HP)
{
    //P -= KHP
    matrix_product(P, K, HP, false, false, 1.0, -1.0);
    // enforce symmetry
    P.map().triangularView<Eigen::Upper>() = P.map().triangularView<Eigen::Lower>().transpose();
}
