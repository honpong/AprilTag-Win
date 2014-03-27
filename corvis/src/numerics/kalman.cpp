// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <assert.h>
#include "kalman.h"
#include "matrix.h"
#include <string.h>
/*
  block update looks like:
  L 0 * A B * L' 0  =  LAL' LB
  0 I   C D   0  0      CL'  D
  C = B'(symmetry of covar matr)

  temp = A + LA'
  B = (CL')' = LB
  C = B'
*/

//TODO: symmetric version?

//TODO: customized blockwise version... use pointers at each block location, null => entire block is zero...?
//TODO: this doesn't even need to be square -- could make ltu rectangular to reduce the wasted cycles.

void block_update(matrix &c, const matrix &lu, const int index)
{
    //symmetric operations, so we use the larger dimension for the upper block
    int asize = lu.cols;
    assert(asize <= c.rows);
    //temp = LA'
    matrix temp(asize, asize);
    matrix A(&c(index,index), asize, asize, c.maxrows, c.stride);
    matrix_product(temp, lu, A, false, true);
    //    matrix_A_dot_Bt_plus_C(&temp, 1.0, lu, &A, 0.0, NULL);
    //A = LAL'
    matrix_product(A, temp, lu, false, true);
    //    matrix_A_dot_Bt_plus_C(&A, 1.0, &temp, lu, 0.0, NULL);

    int bsize = c.rows - index - asize;
    if(bsize > 0) {
        //B += L(B')'
        matrix B(&c(index, index + asize), asize, bsize, c.maxrows, c.stride);
        matrix C(&c(index + asize, index), bsize, asize, c.maxrows, c.stride);
        matrix_product(B, lu, C, false, true);
        //        matrix_A_dot_Bt_plus_C(&B, 1.0, lu, &C, 0.0, NULL);
        //B' = (B)'
        matrix_transpose(C, B);
    }
    bsize = index;
    if(bsize > 0) {
        //B += L(B')'
        matrix B(&c(index, 0), asize, bsize, c.maxrows, c.stride);
        matrix C(&c(0, index), bsize, asize, c.maxrows, c.stride);
        matrix_product(B, lu, C, false, true);
        //        matrix_A_dot_Bt_plus_C(&B, 1.0, lu, &C, 0.0, NULL);
        //B' = (B)'
        matrix_transpose(C, B);
    }
}

void time_update(matrix &c, const matrix &ltu_, const matrix &p_cov, const f_t dt)
{
    block_update(c, ltu_, 0);
    /*MAT_TEMP(LC, c.rows, ltu_.cols);
    matrix_product(LC, ltu_, c, false, true);
    matrix_product(c, ltu_, LC, false, true);*/

    //cov += diag(R)*dt
    for(int i = 0; i < c.rows; ++i) {
        c(i, i) += p_cov[i] * dt;
    }
}

//TODO: implement IEKF sparse update.
//TODO: woodbury matrix identity:
//      start with inv (lambda) = 1/R (diagonal R)
//      update 1 measurement at a time ... this is only good if C is much smaller than meas! -- have to invert C
//      maybe with information filter?
//TODO: block update as above... once we calculate the kalman gain, we multiply by
// the linearization, giving us a gamma which is nicely blocked.
void meas_update(matrix &state, matrix &cov, const matrix &innov, const matrix &lp, const matrix &m_cov)
{
    /*    MAT_TEMP(Cmt, cov.rows, lp.rows);
    matrix_product(Cmt, cov, lp, false, true);
    MAT_TEMP(lambda, lp.rows, Cmt.cols);
    matrix_product(lambda, lp, Cmt);
    for(int i = 0; i < m_cov.cols; ++i)
        lambda(i,i) += m_cov[i];
    //TODO cholesky solve instead of symmetric? how can i tell?
    matrix_solve_syt(lambda, Cmt);
    MAT_TEMP(GAMMA, state.cols, state.cols);
    matrix_product(GAMMA, Cmt, lp, false, false, 0., -1.);
    for(int i = 0; i < GAMMA.rows; ++i)
        GAMMA(i,i) += 1.;
    matrix_product(state, innov, Cmt, false, true, 1.0);
    MAT_TEMP(GC, state.cols, state.cols);
    matrix_product(GC, GAMMA, cov, false, true);
    matrix_product(cov, GAMMA, GC, false, true);
    
    MAT_TEMP(R, m_cov.cols, m_cov.cols);
    memset(R_data, 0, sizeof(R_data));
    for(int i = 0; i < m_cov.cols; ++i)
        R(i, i) = m_cov[i];

    MAT_TEMP(CmtR, state.cols, m_cov.cols);
    matrix_product(CmtR, Cmt, R, false, true);
    matrix_product(cov, Cmt, CmtR, false, true, 1.0);
        
    return;*/
    //no partial blocks
    //temp = A + LA'
    matrix LCt(lp.rows, lp.cols);
    matrix A(cov.data, lp.cols, lp.cols, cov.maxrows, cov.stride);
    matrix_product(LCt, lp, A, false, true);

    //LC = LC'
    //MAT_TEMP(LCt, lp.rows, cov.cols);
    //matrix_A_dot_Bt_plus_C(&LCt, 1.0, lp, cov, 0.0, NULL);
    matrix lambda(lp.rows, lp.rows);
    memset(lambda.data, 0, lambda.stride * lambda.maxrows);
    //lambda = R
    //lambda += LCL'
    for(int i = 0; i < lp.rows; ++i) {
        lambda(i, i) = m_cov[i];
    }
    matrix_product(lambda, LCt, lp, false, true, 1.0);
    //    matrix_A_dot_Bt_plus_C(&lambda, 1.0, &LCt, lp, 1.0, &lambda);
    matrix K(lp.cols, lp.rows);
    //lambda K = CL'
    matrix_transpose(K, LCt);
    matrix_solve(lambda, K);
    //state.T += innov.T * K.T
    matrix_product(state, innov, K, false, true, 1.0);
    //gamma = KL
    matrix gamma(lp.cols, lp.cols);
    memset(gamma.data, 0, gamma.stride * gamma.maxrows);
    for(int i = 0; i < lp.cols; ++i) {
        gamma(i, i) = 1.;
    };
    //TODO: matrix_At_dot_B
    matrix_product(gamma, K, lp, false, false, 1.0, -1.0);
    //matrix_A_dot_Bt_plus_C(&gamma, -1.0, &K, &lpt, 1.0, &gamma);

    block_update(cov, gamma);
    //TODO: make the I implicit and use block update
    matrix KR(K.rows, K.cols);
    //Kr = KR
    assert(K.cols == m_cov.cols);
    for(int i = 0; i < K.rows; ++i) {
        for(int j = 0; j < K.cols; ++j) {
            KR(i, j) = K(i, j) * m_cov[j];
        }
    }
    //Pp += KRK'
    matrix_product(A, KR, K, false, true, 1.0);
    /*int statesize = lp.cols;
    int meas_size = lp.rows;

    //project state cov onto measurement to get cov(meas, state)
    MAT_TEMP(LC, meas_size, statesize);
    matrix A(cov.data, lp.cols, lp.cols, cov.maxrows, cov.stride);
    matrix_product(LC, lp, A, false, false);
    
    //project cov(state, meas)=(LC)' onto meas to get cov(meas, meas), and add measurement cov to get residual covariance
    MAT_TEMP(res_cov, meas_size, meas_size);
    matrix_product(res_cov, lp, LC, false, true);
    for(int i = 0; i < lp.rows; ++i) {
        res_cov(i, i) += m_cov[i];
    }

    MAT_TEMP(K, lp.cols, lp.rows);
    //lambda K = CL'
    matrix_transpose(K, LC);
    matrix_solve(res_cov, K);
    //state.T += innov.T * K.T
    matrix_product(state, innov, K, false, true, 1.0);
    //cov -= KHP
    matrix_product(A, K, LC, false, false, 1.0, -1.0);*/
}

bool kalman_compute_gain(matrix &gain, const matrix &LC, const matrix &inn_cov)
{
    gain.resize(LC.cols, LC.rows);
    //lambda K = CL'
    matrix_transpose(gain, LC);
    matrix factor(inn_cov.rows, inn_cov.cols);
    for(int i = 0; i < inn_cov.rows; ++i) {
        for(int j = 0; j < inn_cov.cols; ++j) {
            factor(i, j) = inn_cov(i, j);
        }
    }
#ifdef TEST_POSDEF
    if(!test_posdef(factor)) { fprintf(stderr, "observation covariance matrix not positive definite before solve!\n"); assert(0); }
#endif
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
    //TODO: look at old meas_udpate inherited from stefano - stable riccatti version?
    //enforce symmetry
    for(int i = 0; i < cov.rows; ++i) {
        for(int j = i + 1; j < cov.cols; ++j) {
            cov(i, j) = cov(j, i) = (cov(i, j) + cov(j, i)) * .5;
        }
    }
}
