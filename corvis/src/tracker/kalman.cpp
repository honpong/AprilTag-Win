// Written by Eagle Jones
// Copyright (c) 2012-2015, RealityCap
// All rights reserved.
//

#include <assert.h>
#include "kalman.h"
#include "matrix.h"
#include <string.h>

bool kalman_compute_gain(matrix &K, const matrix &HP, const matrix &S, matrix &tmp)
{
    //K = P * H' * inv(S)
    //Lapack uses column-major ordering, so we feed it PH' and get K directly, in the form X * A = B' rather than A' * X = B
    K.resize(HP.cols(), HP.rows());
    K.map() = HP.map().transpose();
    tmp.resize(S.rows(), S.cols());
    tmp.map() = S.map();
    return matrix_solve(tmp, K); // K = K inv(tmp), tmp is destroyed
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

void kalman_update_covariance_robust(matrix &P, const matrix &K, const matrix &HP, const matrix &S)
{
    matrix KHP(P.rows(), P.cols());
    matrix_product(KHP, K, HP);
    matrix KS(K.rows(), S.cols());
    //Could optimize this to use dsymm rather than dgemm
    matrix_product(KS, K, S);
    matrix KSKt(P.rows(), P.cols());
    matrix_product(KSKt, KS, K, false, true);
    for(int i = 0; i < P.rows(); ++i) {
        for(int j = 0; j < i; ++j) {
            P(i, j) = P(j, i) = P(i, j) - ((KHP(i, j) + KHP(j, i)) - KSKt(i, j));
        }
        P(i, i) = P(i, i) - ((KHP(i, i) + KHP(i, i)) - KSKt(i, i));
    }
}

