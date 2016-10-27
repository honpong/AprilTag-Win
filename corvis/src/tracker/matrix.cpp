// Copyright (c) 2008-2012, Eagle Jones
// Copyright (c) 2012-2015 RealityCap, Inc
// All rights reserved.
//

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "matrix.h"
#include <math.h>

#ifndef WIN32
#include <alloca.h>
#endif

#include <Eigen/Cholesky>

#ifdef F_T_IS_DOUBLE
#define F_T_TYPE(prefix,func,suffix) prefix ## d ## func ## suffix
#else
#define F_T_TYPE(prefix,func,suffix) prefix ## s ## func ## suffix
#endif

#define LAPACK_(func) F_T_TYPE(      ,func, _)
#define cblas_(func)  F_T_TYPE(cblas_,func,  )

#define walloca(t,n) (t*)alloca(sizeof(t)*(n))

#include <stdio.h>

bool matrix::is_symmetric(f_t eps = 1.e-5) const
{
    for(int i = 0; i < _rows; ++i) {
        for(int j = i; j < _cols; ++j) {
            if(fabs((*this)(i,j) - (*this)(j,i)) > eps) return false;
        }
    }
    return true;
}

void matrix::print() const
{
    for(int i = 0; i < _rows; ++i) {
        for(int j = 0; j < _cols; ++j) {
            f_t v = (*this)(i, j);
            if(i == j) fprintf(stderr, "[% .1e]", v);
            else if(v == 0) fprintf(stderr, "     0    ");
            else fprintf(stderr, " % .1e ", v);
        }
        fprintf(stderr, "\n");
    }
}

void matrix::print_high() const
{
    for(int i = 0; i < _rows; ++i) {
        for(int j = 0; j < _cols; ++j) {
            f_t v = (*this)(i, j);
            if(i == j) fprintf(stderr, "[% .10e]", v);
            else if(v == 0) fprintf(stderr, "     0    ");
            else fprintf(stderr, " % .10e ", v);
        }
        fprintf(stderr, "\n");
    }
}

void matrix::print_diag() const
{
    for(int i = 0; i < _rows; ++i) {
        fprintf(stderr, "% .1e ", (*this)(i, i));
    }
    fprintf(stderr, "\n");
}

void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1, bool trans2, const f_t dst_scale, const f_t scale)
{
    assert(dst_scale == 1); // a limitation of Eigen's GEMM detection
    if(!trans1 && !trans2)
        res.map().noalias() += scale*A.map()             * B.map();
    else if(trans1 && !trans2)
        res.map().noalias() += scale*A.map().transpose() * B.map();
    else if(!trans1 && trans2)
        res.map().noalias() += scale*A.map()             * B.map().transpose();
    else
        res.map().noalias() += scale*A.map().transpose() * B.map().transpose();
}

f_t matrix_check_condition(matrix &A)
{
    return A.map().llt().rcond();
}

bool matrix_solve(matrix &A, matrix &B)
{
    Eigen::LLT<Eigen::Matrix<f_t,Eigen::Dynamic,Eigen::Dynamic>> llt(A.map());
    if (llt.info() == Eigen::NumericalIssue)
        return false;
    Eigen::Map<Eigen::Matrix<f_t,Eigen::Dynamic,Eigen::Dynamic,Eigen::ColMajor>,Eigen::Unaligned,Eigen::OuterStride<>>
        Bt(B.data, B.cols(), B.rows(),Eigen::OuterStride<>(B.get_stride()));
    llt.solveInPlace(Bt);
    return true;
}

void matrix_transpose(matrix &dst, const matrix &src)
{
    assert(dst._cols == src._rows && dst._rows == src._cols);
    for(int i = 0; i < dst._rows; ++i) {
        for(int j = 0; j < dst._cols; ++j) {
            dst(i, j) = src(j, i);
        }
    }
}

//need to put this test around every operation that affects cov. (possibly with #defines, google test?)
bool test_posdef(const matrix &m)
{
    f_t norm = (m.map() - m.map().transpose()).norm();
    if(norm > F_T_EPS)
        return false;
    return m.map().llt().info() != Eigen::NumericalIssue;
}
