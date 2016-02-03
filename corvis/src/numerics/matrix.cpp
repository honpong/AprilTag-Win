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

#if defined(__APPLE__) && !defined(__INTEL_COMPILER)
#include <Accelerate/Accelerate.h>
#define lapack_int __CLPK_integer
#else // __APPLE__
#if defined(WIN32) || defined(ANDROID) || defined(__INTEL_COMPILER)
#include <mkl.h>
#else // WIN32
#include <cblas.h>
#include <lapacke.h>
#endif
#endif

#ifdef F_T_IS_SINGLE
#define F_T_TYPE(prefix,func,suffix) prefix ## s ## func ## suffix
#endif
#ifdef F_T_IS_DOUBLE
#define F_T_TYPE(prefix,func,suffix) prefix ## d ## func ## suffix
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

bool matrix_invert(matrix &m)
{
    char uplo = 'U';
    lapack_int * ipiv = walloca(lapack_int, m.stride);
    //just make a work array as big as the input
    //TODO: call ssytrf_ with lwork = -1 -- returns optimal size as work[0] ? wtf?
    lapack_int ign = -1;
    const char *name = "DSYTRF";
    const char *tp = "U";
    lapack_int ispec = 1;
    lapack_int n = m._cols;
    lapack_int lda = m.stride;
    lapack_int lwork = m.stride * ilaenv_(&ispec, (char *)name, (char *)tp, &n, &ign, &ign, &ign);
    if(lwork < 1) lwork = m.stride*4;
    lapack_int * work = walloca(lapack_int, lwork);
    lapack_int info;
    LAPACK_(sytrf)(&uplo, &n, m.data, &lda, ipiv, (f_t *)work, &lwork, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "matrix_invert: ssytrf failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    LAPACK_(sytri)(&uplo, &n, m.data, &lda, ipiv, (f_t *)work, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "matrix_invert: ssytri failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    //only generates half the matrix. so-called upper is actually lower (fortran)
    for(int i = 0; i < m._rows; ++i) {
        for(int j = i+1; j < m._rows; ++j) {
            m(i, j) = m(j, i);
        }
    }
    return true;
}

void matrixest();
bool matrix_solve_syt(matrix &A, matrix &B)
{
    char uplo = 'U';
    lapack_int * ipiv = walloca(lapack_int, A.stride);
    //just make a work array as big as the input
    //TODO: call ssytrf_ with lwork = -1 -- returns optimal size as work[0] ? wtf?
    lapack_int ign = -1;
    const char *name = "DSYTRF";
    const char *tp = "U";
    lapack_int ispec = 1;
    lapack_int n = A._cols;
    lapack_int lda = A.stride;
    lapack_int lwork = A.stride * ilaenv_(&ispec, (char *)name, (char *)tp, &n, &ign, &ign, &ign);
    if(lwork < 1) lwork = A.stride*4;
    f_t * work = walloca(f_t, lwork);
    lapack_int info;
    LAPACK_(sytrf)(&uplo, &n, A.data, &lda, ipiv, work, &lwork, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "matrix_solve: sytrf failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    lapack_int nrhs = B._rows;
    lapack_int ldb = B.stride;
    LAPACK_(sytrs)(&uplo, &n, &nrhs, A.data, &lda, ipiv, B.data, &ldb, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "matrix_solve: sytrs failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    return true;
}

/*
bool matrix_solve_semidefinite(matrix &A, matrix &B)
{
    assert("Not working - pivot order is wrong?" & 0);
    char uplo = 'U';
    lapack_int piv[A.cols];
    f_t work[2 * A.cols];
    lapack_int info;
    f_t tol = -1.;
    lapack_int rank;
    lapack_int n = A.cols;
    lapack_int lda = A.stride;
    LAPACK_(pstrf)(&uplo, &n, A.data, &lda, piv, &rank, &tol, work, &info);
    if(info) {
        fprintf(stderr, "matrix_solve: pstrf failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    lapack_int k1 = 1;
    lapack_int inc = 1;
    lapack_int nrhs = B.rows;
    lapack_int ldb = B.stride;
    lapack_int Bcols = B.cols;
    laswp(&Bcols, B.data, &ldb, &k1, &n, piv, &inc);
    LAPACK_(potrs)(&uplo, &n, &nrhs, A.data, &lda, B.data, &ldb, &info);
    if(info) {
        fprintf(stderr, "solve: spotrs failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    inc = -1;
    laswp(&Bcols, B.data, &ldb, &k1, &n, piv, &inc);
    return true;
}
*/

void test_cholesky(matrix &A)
{
    assert(A._rows = A._cols);
    assert(A.is_symmetric());
    lapack_int N = A._rows;
    matrix res(N, N);
    matrix B(N, N);
    fprintf(stderr, "original matrix is: \n");
    A.print();


    for (int i = 0; i < N; ++i) {
        for(int j = 0; j < N; ++j) {
            B(i, j) = A(i, j);
        }
    }

    char uplo = 'U';
    lapack_int info;
    lapack_int ldb;
    LAPACK_(potrf)(&uplo, &N, B.data, &ldb, &info);
    if(info) {
        fprintf(stderr, "cholesky: potrf failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
    }
    fprintf(stderr, "cholesky result is: \n");
    B.print();
    //clear out any leftover data in the upper part of B
    for(int i = 0; i < N; ++i) {
        for(int j = i + 1; j < N; ++j) {
            B(i, j) = 0.;
        }
    }
    fprintf(stderr, "after clearing: \n");
    B.print();
    matrix_product(res, B, B, false, true);
    fprintf(stderr, "after multiplication: \n");
    res.print();
    for (int i = 0; i < N; ++i) {
        for(int j = 0; j < N; ++j) {
            /*if(fabs(res(i, j) - A(i,j)) > 1.e-45) {
                            fprintf(stderr, "(%d, %d) res is %e, cov is %e\n", i, j, res(i,j), A(i,j));
                            }*/
            B(i, j) = res(i, j) - A(i, j);
        }
    }
    fprintf(stderr, "residual is: \n");
    B.print();
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

bool matrix_solve_refine(matrix &A, matrix &B)
{
    char uplo = 'U';
    lapack_int info;
    lapack_int n = A._cols;
    lapack_int lda = A.stride;
    matrix AF(A._rows, A._cols);
    for(int i = 0; i < A._rows; ++i) {
        for(int j = 0; j < A._cols; ++j) {
            AF(i, j) = A(i, j);
        }
    }
    lapack_int ldaf = A.stride;
    LAPACK_(potrf)(&uplo, &n, AF.data, &ldaf, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "solve: spotrf failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false; //could return matrix_solve_syt here instead
    }
    lapack_int nrhs = B._rows;
    lapack_int ldb = B.stride;
    matrix X(B._rows, B._cols);
    for(int i = 0; i < B._rows; ++i) {
        for(int j = 0; j < B._cols; ++j) {
            X(i, j) = B(i, j);
        }
    }
    lapack_int ldx = X.stride;
    LAPACK_(potrs)(&uplo, &n, &nrhs, AF.data, &ldaf, X.data, &ldx, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "solve: spotrs failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    f_t *ferr = walloca(f_t, nrhs), *berr = walloca(f_t,nrhs);
    lapack_int *iwork = walloca(lapack_int, n);
    f_t *work = walloca(f_t, 3 * n);
    LAPACK_(porfs)(&uplo, &n, &nrhs, A.data, &lda, AF.data, &ldaf, B.data, &ldb, X.data, &ldx, ferr, berr, work, iwork, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "solve: porfs failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    for(int i = 0; i < B._rows; ++i) {
        for(int j = 0; j < B._cols; ++j) {
            B(i, j) = X(i, j);
        }
    }
    
    return true;
}

bool matrix_solve_extra(matrix &A, matrix &B)
{
    char fact = 'E';
    char uplo = 'U';
    char equed = 'N';
    lapack_int info;
    lapack_int n = A._cols;
    lapack_int lda = A.stride;
    matrix AF(A._rows, A._cols);
    lapack_int ldaf = A.stride;
    lapack_int nrhs = B._rows;
    lapack_int ldb = B.stride;
    matrix X(B._rows, B._cols);
    lapack_int ldx = X.stride;
    f_t *ferr = walloca(f_t,nrhs), *berr = walloca(f_t,nrhs);
    lapack_int *iwork = walloca(lapack_int, n);
    f_t *work = walloca(f_t, 3 * n);
    f_t *s = walloca(f_t, n);
    f_t rcond;
    LAPACK_(posvx)(&fact, &uplo, &n, &nrhs, A.data, &lda, AF.data, &ldaf, &equed, s, B.data, &ldb, X.data, &ldx, &rcond, ferr, berr, work, iwork, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "solve: posvx failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    for(int i = 0; i < B._rows; ++i) {
        for(int j = 0; j < B._cols; ++j) {
            B(i, j) = X(i, j);
        }
    }
    
    return true;
}

bool matrix_solve_svd(matrix &A, matrix &B)
{
    lapack_int info;
    f_t *sv = walloca(f_t, A._rows);
    lapack_int rank;
    f_t rcond = -1;
    lapack_int lwork = -1;
    lapack_int n = A._cols;
    lapack_int lda = A.stride;
    lapack_int nrhs = B._rows;
    lapack_int ldb = B.stride;
    f_t work0;
    LAPACK_(gelsd)(&n, &n, &nrhs, A.data, &lda, B.data, &ldb, sv, &rcond, &rank, &work0, &lwork, 0, &info);
    lwork = (int)work0;
    f_t *work = walloca(f_t, lwork);
    lapack_int *iwork = walloca(lapack_int, lwork);
    LAPACK_(gelsd)(&n, &n, &nrhs, A.data, &lda, B.data, &ldb, sv, &rcond, &rank, work, &lwork, iwork, &info);

    //fprintf(stderr, "svd reported rank: %ld\n", rank);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "solve: sgelsd failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    return true;
}


//returns in new matrices, but destroys A
// S should have dimension at least max(1, min(m,n))
bool matrix_svd(matrix &A, matrix &U, matrix &S, matrix &Vt)
{
    lapack_int info;

    lapack_int lwork = -1;
    f_t work0;
    lapack_int *iwork = walloca(lapack_int, 8 * A._cols);
    //gesvd/dd is fortran, so V^T and U are swapped
    lapack_int n = A._rows;
    lapack_int m = A._cols;
    lapack_int lda = A.stride;
    lapack_int ldVt = Vt.stride;
    lapack_int ldU = U.stride;

    LAPACK_(gesdd)((char *)"A", &m, &n, A.data, &lda, S.data, Vt.data, &ldVt, U.data, &ldU, &work0, &lwork, iwork, &info);
    lwork = (int)work0;
    f_t *work = walloca(f_t, lwork);
    LAPACK_(gesdd)((char *)"A", &m, &n, A.data, &lda, S.data, Vt.data, &ldVt, U.data, &ldU, work, &lwork, iwork, &info);
    if(info) {
#ifdef DEBUG
        fprintf(stderr, "svd: gesvd failed: %d\n", (int)info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
#endif
        return false;
    }
    return true;
}

void matrix_negate(matrix & mat)
{
    for(int r = 0; r < mat._rows; r++) {
        for(int c = 0; c < mat._cols; c++) {
            mat(r,c) = -mat(r,c);
        }
    }
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

matrix &matrix_dereference(matrix *m)
{
    return *m;
}

f_t matrix_3x3_determinant(const matrix & A)
{
    assert(A._cols == 3 && A._rows == 3);

    return A(0,0)*(A(1,1)*A(2,2) - A(1,2)*A(2,1)) -
           A(0,1)*(A(1,0)*A(2,2) - A(1,2)*A(2,0)) +
           A(0,2)*(A(1,0)*A(2,1) - A(1,1)*A(2,0));
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
