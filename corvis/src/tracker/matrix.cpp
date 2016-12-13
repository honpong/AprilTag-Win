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

#ifdef HAVE_MKL
#include <mkl.h>
#elif defined(__APPLE__)
#include <Accelerate/Accelerate.h>
#define lapack_int __CLPK_integer
#else
#include <cblas.h>
#include <lapacke.h>
#endif

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
    int d1, d2, d3, d4;
    if(trans1) {
        d1 = A._cols;
        d2 = A._rows;
    } else {
        d1 = A._rows;
        d2 = A._cols;
    }
    if(trans2) {
        d3 = B._cols;
        d4 = B._rows;
    } else {
        d3 = B._rows;
        d4 = B._cols;
    }
    assert(d2 == d3);
    assert(res._rows == d1);
    assert(res._cols == d4);
    cblas_(gemm)(CblasRowMajor, trans1?CblasTrans:CblasNoTrans, trans2?CblasTrans:CblasNoTrans,
                 res._rows, res._cols, A._cols,
                 scale, A.data, A.stride,
                 B.data, B.stride,
                 dst_scale, res.data, res.stride);
}

//returns lower triangular (by my conventions) cholesky matrix
bool matrix_cholesky(matrix &A)
{
    //test_cholesky(A);
    //A.print();
    char uplo = 'U';
    lapack_int info;
    lapack_int n = A._cols;
    lapack_int lda = A.stride;
    LAPACK_(potrf)(&uplo, &n, A.data, &lda, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "cholesky: potrf failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    //potrf only computes upper fortran (so really lower) triangle
    //clear out any leftover data in the upper part of A
    for(int i = 0; i < A._rows; ++i) {
        for(int j = i + 1; j < A._rows; ++j) {
            A(i, j) = 0.;
        }
    }
    return true;
}

f_t matrix_check_condition(matrix &A)
{
    f_t anorm = 0.;
    
    matrix tmp(A._rows, A._cols);

    for(int i = 0; i < A._rows; ++i)
    {
        f_t sum = 0.;
        for(int j = 0; j < A._cols; ++j)
        {
            sum += A(i, j);
            tmp(i, j) = A(i, j);
        }
        if(sum > anorm) anorm = sum;
    }
    
    char uplo = 'U';
    lapack_int info;
    lapack_int n = tmp._cols;
    lapack_int lda = tmp.stride;
    LAPACK_(potrf)(&uplo, &n, tmp.data, &lda, &info);
    f_t rcond = 1.;
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "check_condition: potrf failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return 0.;
    }
    
    lapack_int *iwork = walloca(lapack_int, n);
    f_t *work = walloca(f_t, 3*n);
    LAPACK_(pocon)(&uplo, &n, tmp.data, &lda, &anorm, &rcond, work, iwork, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "check_condition: pocon failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return 0.;
    }
    return rcond;
}

bool matrix_solve(matrix &A, matrix &B)
{
    char uplo = 'U';
    lapack_int info;
    lapack_int n = A._cols;
    lapack_int lda = A.stride;
    LAPACK_(potrf)(&uplo, &n, A.data, &lda, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "solve: spotrf failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false; //could return matrix_solve_syt here instead
    }
    lapack_int nrhs = B._rows;
    lapack_int ldb = B.stride;
    LAPACK_(potrs)(&uplo, &n, &nrhs, A.data, &lda, B.data, &ldb, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "solve: spotrs failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
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
    matrix tmp(m._rows, m._cols);
    bool ret = true;
    for(int i = 0; i < m._rows; ++i)
    {
        if(m(i, i) < 0.) {
            fprintf(stderr, "negative diagonal element: %d is %e\n", i, m(i, i));
            ret = false;
        }
        tmp(i, i) = m(i, i);
        for(int j = i + 1; j < m._cols; ++j)
        {
            if(m(i, j) != m(j, i)) {
                fprintf(stderr, "not symmetric: m(%d, %d) = %e; m(%d, %d) = %e\n", i, j, m(i, j), j, i, m(j, i));
                ret = false;
            }
            tmp(i, j) = tmp(j, i) = m(i, j);
        }
    }
    if(ret) ret = matrix_cholesky(tmp);
    if(!ret)
    {
        m.print();
        return false;
    }
    return ret;
}
