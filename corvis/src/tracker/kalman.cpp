// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#include <assert.h>
#include "kalman.h"
#include "matrix.h"
#include <string.h>

bool kalman_compute_gain(matrix &gain, const matrix &LC, const matrix &inn_cov)
{
    //K = CL' * inv(res_cov)
    //K * res_cov = CL'
    //Lapack uses column-major ordering, so we feed it CL' and get K directly, in the form X * A = B' rather than A' * X = B
    gain.resize(LC.cols(), LC.rows());
    matrix_transpose(gain, LC);
    matrix factor(inn_cov.rows(), inn_cov.cols());
    for(int i = 0; i < inn_cov.rows(); ++i) {
        for(int j = 0; j < inn_cov.cols(); ++j) {
            factor(i, j) = inn_cov(i, j);
        }
    }
    return matrix_solve(factor, gain);
}

void kalman_update_state(matrix &state, const matrix &gain, const matrix &inn)
{
    //state.T += innov.T * K.T
    matrix_product(state, inn, gain, false, true, 1.0);
}

void kalman_update_covariance(matrix &cov, const matrix &gain, const matrix &LC)
{
    //cov -= KHP
    matrix_product(cov, gain, LC, false, false, 1.0, -1.0);
    //enforce symmetry
    for(int i = 0; i < cov.rows(); ++i) {
        for(int j = i + 1; j < cov.cols(); ++j) {
            cov(i, j) = cov(j, i) = (cov(i, j) + cov(j, i)) * f_t(.5);
        }
    }
}

void kalman_update_covariance_robust(matrix &cov, const matrix &gain, const matrix &LC, const matrix inn_cov)
{
    matrix KLCterm(cov.rows(), cov.cols());
    matrix_product(KLCterm, gain, LC);
    matrix KS(gain.rows(), inn_cov.cols());
    //Could optimize this to use dsymm rather than dgemm
    matrix_product(KS, gain, inn_cov);
    matrix KSK(cov.rows(), cov.cols());
    matrix_product(KSK, KS, gain, false, true);
    for(int i = 0; i < cov.rows(); ++i) {
        for(int j = 0; j < i; ++j) {
            cov(i, j) = cov(j, i) = cov(i, j) - ((KLCterm(i, j) + KLCterm(j, i)) - KSK(i, j));
        }
        cov(i, i) = cov(i, i) - ((KLCterm(i, i) + KLCterm(i, i)) - KSK(i, i));
    }
}

