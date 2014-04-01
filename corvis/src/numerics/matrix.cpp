// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "matrix.h"
#include <math.h>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#else
#include <cblas.h>
#include <clapack.h>
#endif

#include <stdio.h>

extern "C" {

#ifndef __APPLE__
    extern int ilaenv_(int *, char *, char *, int *, int *, int *, int *, int, int);
    extern int ssytrf_(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *lwork, int *info);
    extern int ssytri_(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *info);
    extern int spotrf_(char *, int *, f_t *, int *, int *);
    extern int spotri_(char *, int *, f_t *, int *, int *);
    extern int spotrs_(char *, int *, int *, f_t *, int *, f_t *, int *, int *);
    extern int ssytrs_(char *, int *, int *, f_t *, int *, int *, f_t *, int *, int *);
    extern int sgelsd_(int *, int *, int *, f_t *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, int *, int *);
    extern int sgesvd_(char *, char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *);
    extern int sgesdd_(char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *, int *);
    
    extern int dsytrf_(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *lwork, int *info);
    extern int dsytri_(char *uplo, int *n, f_t *a, int *lda, int *ipiv, f_t *work, int *info);
    extern int dpotrf_(char *, int *, f_t *, int *, int *);
    extern int dpotri_(char *, int *, f_t *, int *, int *);
    extern int dpotrs_(char *, int *, int *, f_t *, int *, f_t *, int *, int *);
    extern int dsytrs_(char *, int *, int *, f_t *, int *, int *, f_t *, int *, int *);
    extern int dgelsd_(int *, int *, int *, f_t *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, int *, int *);
    extern int dgesvd_(char *, char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *);
    extern int dgesdd_(char *, int *, int *, f_t *, int *, f_t *, f_t *, int *, f_t *, int *, f_t *, int *, int *, int *);
#endif

#ifdef F_T_IS_SINGLE
    static int (*sytrf)(char *uplo, __CLPK_integer *n, f_t *a, __CLPK_integer *lda, __CLPK_integer *ipiv, f_t *work, __CLPK_integer *lwork, __CLPK_integer *info) = ssytrf_;
    static int (*sytri)(char *uplo, __CLPK_integer *n, f_t *a, __CLPK_integer *lda, __CLPK_integer *ipiv, f_t *work, __CLPK_integer *info) = ssytri_;
    static int (*sytrs)(char *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = ssytrs_;
    
    static int (*pstrf)(char *uplo, __CLPK_integer *n, f_t *a, __CLPK_integer *lda, __CLPK_integer *piv, __CLPK_integer *rank, f_t *tol, f_t * work, __CLPK_integer *info) = spstrf_;
    
    static int (*laswp)(__CLPK_integer *n, f_t *a, __CLPK_integer *lda, __CLPK_integer *k1, __CLPK_integer *k2, __CLPK_integer *piv, __CLPK_integer *inc) = slaswp_;
    
    static int (*potrf)(char *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = spotrf_;
    static int (*pocon)(char *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, f_t *, f_t *, __CLPK_integer *, __CLPK_integer *) = spocon_;
    static int (*potri)(char *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = spotri_;
    static int (*potrs)(char *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = spotrs_;
    
    static int (*gelsd)(__CLPK_integer *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *, __CLPK_integer *) = sgelsd_;
    static int (*gesvd)(char *, char *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = sgesvd_;
    static int (*gesdd)(char *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *, __CLPK_integer *) = sgesdd_;

#endif
#ifdef F_T_IS_DOUBLE
    static int (*sytrf)(char *uplo, __CLPK_integer *n, f_t *a, __CLPK_integer *lda, __CLPK_integer *ipiv, f_t *work, __CLPK_integer *lwork, __CLPK_integer *info) = dsytrf_;
    static int (*sytri)(char *uplo, __CLPK_integer *n, f_t *a, __CLPK_integer *lda, __CLPK_integer *ipiv, f_t *work, __CLPK_integer *info) = dsytri_;
    static int (*sytrs)(char *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = dsytrs_;
    
    static int (*pstrf)(char *uplo, __CLPK_integer *n, f_t *a, __CLPK_integer *lda, __CLPK_integer *piv, __CLPK_integer *rank, f_t *tol, f_t * work, __CLPK_integer *info) = dpstrf_;

    static int (*laswp)(__CLPK_integer *n, f_t *a, __CLPK_integer *lda, __CLPK_integer *k1, __CLPK_integer *k2, __CLPK_integer *piv, __CLPK_integer *inc) = dlaswp_;

    static int (*potrf)(char *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = dpotrf_;
    static int (*pocon)(char *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, f_t *, f_t *, __CLPK_integer *, __CLPK_integer *) = dpocon_;
    static int (*potri)(char *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = dpotri_;
    static int (*potrs)(char *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = dpotrs_;
    
    static int (*gelsd)(__CLPK_integer *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *, __CLPK_integer *) = dgelsd_;
    static int (*gesvd)(char *, char *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *) = dgesvd_;
    static int (*gesdd)(char *, __CLPK_integer *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, f_t *, __CLPK_integer *, __CLPK_integer *, __CLPK_integer *) = dgesdd_;
#endif
}

bool matrix::is_symmetric(f_t eps = 1.e-5) const
{
    for(int i = 0; i < rows; ++i) {
        for(int j = i; j < cols; ++j) {
            if(fabs((*this)(i,j) - (*this)(j,i)) > eps) return false;
        }
    }
    return true;
}

void matrix::print() const
{
    for(int i = 0; i < rows; ++i) {
        for(int j = 0; j < cols; ++j) {
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
    for(int i = 0; i < rows; ++i) {
        for(int j = 0; j < cols; ++j) {
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
    for(int i = 0; i < rows; ++i) {
        fprintf(stderr, "% .1e ", (*this)(i, i));
    }
    fprintf(stderr, "\n");
}

void matrix_product(matrix &res, const matrix &A, const matrix &B, bool trans1, bool trans2, const f_t dst_scale, const f_t scale)
{
    int d1, d2, d3, d4;
    if(trans1) {
        d1 = A.cols;
        d2 = A.rows;
    } else {
        d1 = A.rows;
        d2 = A.cols;
    }
    if(trans2) {
        d3 = B.cols;
        d4 = B.rows;
    } else {
        d3 = B.rows;
        d4 = B.cols;
    }
    assert(d2 == d3);
    assert(res.rows == d1);
    assert(res.cols == d4);
#ifdef F_T_IS_DOUBLE
    cblas_dgemm(CblasRowMajor, trans1?CblasTrans:CblasNoTrans, trans2?CblasTrans:CblasNoTrans, 
                res.rows, res.cols, A.cols, 
                scale, A.data, A.stride,
                B.data, B.stride, 
                dst_scale, res.data, res.stride);
#else
    cblas_sgemm(CblasRowMajor, trans1?CblasTrans:CblasNoTrans, trans2?CblasTrans:CblasNoTrans, 
                res.rows, res.cols, A.cols, 
                scale, A.data, A.stride,
                B.data, B.stride, 
                dst_scale, res.data, res.stride);
#endif
}    

bool matrix_invert(matrix &m)
{
    char uplo = 'U';
    __CLPK_integer ipiv[m.stride];
    //just make a work array as big as the input
    //TODO: call ssytrf_ with lwork = -1 -- returns optimal size as work[0] ? wtf?
    __CLPK_integer ign = -1;
    const char *name = "DSYTRF";
    const char *tp = "U";
    __CLPK_integer ispec = 1;
    __CLPK_integer n = m.cols;
    __CLPK_integer lda = m.stride;
    __CLPK_integer lwork = m.stride * ilaenv_(&ispec, (char *)name, (char *)tp, &n, &ign, &ign, &ign);
    if(lwork < 1) lwork = m.stride*4;
    __CLPK_integer work[lwork];
    __CLPK_integer info;
    sytrf(&uplo, &n, m.data, &lda, ipiv, (f_t *)work, &lwork, &info);
    if(info) {
        fprintf(stderr, "matrix_invert: ssytrf failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    sytri(&uplo, &n, m.data, &lda, ipiv, (f_t *)work, &info);
    if(info) {
        fprintf(stderr, "matrix_invert: ssytri failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    //only generates half the matrix. so-called upper is actually lower (fortran)
    for(int i = 0; i < m.rows; ++i) {
        for(int j = i+1; j < m.rows; ++j) {
            m(i, j) = m(j, i);
        }
    }
    return true;
}

void matrixest();
bool matrix_solve_syt(matrix &A, matrix &B)
{
    char uplo = 'U';
    __CLPK_integer ipiv[A.stride];
    //just make a work array as big as the input
    //TODO: call ssytrf_ with lwork = -1 -- returns optimal size as work[0] ? wtf?
    __CLPK_integer ign = -1;
    const char *name = "DSYTRF";
    const char *tp = "U";
    __CLPK_integer ispec = 1;
    __CLPK_integer n = A.cols;
    __CLPK_integer lda = A.stride;
    __CLPK_integer lwork = A.stride * ilaenv_(&ispec, (char *)name, (char *)tp, &n, &ign, &ign, &ign);
    if(lwork < 1) lwork = A.stride*4;
    f_t work[lwork];
    __CLPK_integer info;
    sytrf(&uplo, &n, A.data, &lda, ipiv, work, &lwork, &info);
    if(info) {
        fprintf(stderr, "matrix_solve: sytrf failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    __CLPK_integer nrhs = B.rows;
    __CLPK_integer ldb = B.stride;
    sytrs(&uplo, &n, &nrhs, A.data, &lda, ipiv, B.data, &ldb, &info);
    if(info) {
        fprintf(stderr, "matrix_solve: sytrs failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    return true;
}

/*
bool matrix_solve_semidefinite(matrix &A, matrix &B)
{
    assert("Not working - pivot order is wrong?" & 0);
    char uplo = 'U';
    __CLPK_integer piv[A.cols];
    f_t work[2 * A.cols];
    __CLPK_integer info;
    f_t tol = -1.;
    __CLPK_integer rank;
    __CLPK_integer n = A.cols;
    __CLPK_integer lda = A.stride;
    pstrf(&uplo, &n, A.data, &lda, piv, &rank, &tol, work, &info);
    if(info) {
        fprintf(stderr, "matrix_solve: pstrf failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    __CLPK_integer k1 = 1;
    __CLPK_integer inc = 1;
    __CLPK_integer nrhs = B.rows;
    __CLPK_integer ldb = B.stride;
    __CLPK_integer Bcols = B.cols;
    laswp(&Bcols, B.data, &ldb, &k1, &n, piv, &inc);
    potrs(&uplo, &n, &nrhs, A.data, &lda, B.data, &ldb, &info);
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
    assert(A.rows = A.cols);
    assert(A.is_symmetric());
    __CLPK_integer N = A.rows;
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
    __CLPK_integer info;
    __CLPK_integer ldb;
    potrf(&uplo, &N, B.data, &ldb, &info);
    if(info) {
        fprintf(stderr, "cholesky: potrf failed: %ld\n", info);
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
    __CLPK_integer info;
    __CLPK_integer n = A.cols;
    __CLPK_integer lda = A.stride;
    potrf(&uplo, &n, A.data, &lda, &info);
    if(info) {
        fprintf(stderr, "cholesky: potrf failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    //potrf only computes upper fortran (so really lower) triangle
    //clear out any leftover data in the upper part of A
    for(int i = 0; i < A.rows; ++i) {
        for(int j = i + 1; j < A.rows; ++j) {
            A(i, j) = 0.;
        }
    }
    return true;
}

f_t matrix_check_condition(matrix &A)
{
    f_t anorm = 0.;
    
    matrix tmp(A.rows, A.cols);

    for(int i = 0; i < A.rows; ++i)
    {
        f_t sum = 0.;
        for(int j = 0; j < A.cols; ++j)
        {
            sum += A(i, j);
            tmp(i, j) = A(i, j);
        }
        if(sum > anorm) anorm = sum;
    }
    
    char uplo = 'U';
    __CLPK_integer info;
    __CLPK_integer n = tmp.cols;
    __CLPK_integer lda = tmp.stride;
    potrf(&uplo, &n, tmp.data, &lda, &info);
    f_t rcond = 1.;
    if(info) {
        fprintf(stderr, "check_condition: potrf failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return 0.;
    }
    
    __CLPK_integer iwork[n];
    f_t work[3*n];
    pocon(&uplo, &n, tmp.data, &lda, &anorm, &rcond, work, iwork, &info);
    if(info) {
        fprintf(stderr, "check_condition: pocon failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return 0.;
    }
    return rcond;
}

bool matrix_solve(matrix &A, matrix &B)
{
    char uplo = 'U';
    __CLPK_integer info;
    __CLPK_integer n = A.cols;
    __CLPK_integer lda = A.stride;
    potrf(&uplo, &n, A.data, &lda, &info);
    if(info) {
        fprintf(stderr, "solve: spotrf failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false; //could return matrix_solve_syt here instead
    }
    __CLPK_integer nrhs = B.rows;
    __CLPK_integer ldb = B.stride;
    potrs(&uplo, &n, &nrhs, A.data, &lda, B.data, &ldb, &info);
    if(info) {
        fprintf(stderr, "solve: spotrs failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    return true;
}

bool matrix_solve_refine(matrix &A, matrix &B)
{
    char uplo = 'U';
    __CLPK_integer info;
    __CLPK_integer n = A.cols;
    __CLPK_integer lda = A.stride;
    matrix AF(A.rows, A.cols);
    for(int i = 0; i < A.rows; ++i) {
        for(int j = 0; j < A.cols; ++j) {
            AF(i, j) = A(i, j);
        }
    }
    __CLPK_integer ldaf = A.stride;
    potrf(&uplo, &n, AF.data, &ldaf, &info);
    if(info) {
        fprintf(stderr, "solve: spotrf failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false; //could return matrix_solve_syt here instead
    }
    __CLPK_integer nrhs = B.rows;
    __CLPK_integer ldb = B.stride;
    matrix X(B.rows, B.cols);
    for(int i = 0; i < B.rows; ++i) {
        for(int j = 0; j < B.cols; ++j) {
            X(i, j) = B(i, j);
        }
    }
    __CLPK_integer ldx = X.stride;
    potrs(&uplo, &n, &nrhs, AF.data, &ldaf, X.data, &ldx, &info);
    if(info) {
        fprintf(stderr, "solve: spotrs failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    f_t ferr[nrhs], berr[nrhs];
    __CLPK_integer iwork[n];
    f_t work[3 * n];
    dporfs_(&uplo, &n, &nrhs, A.data, &lda, AF.data, &ldaf, B.data, &ldb, X.data, &ldx, ferr, berr, work, iwork, &info);
    if(info) {
        fprintf(stderr, "solve: porfs failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    for(int i = 0; i < B.rows; ++i) {
        for(int j = 0; j < B.cols; ++j) {
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
    __CLPK_integer info;
    __CLPK_integer n = A.cols;
    __CLPK_integer lda = A.stride;
    matrix AF(A.rows, A.cols);
    __CLPK_integer ldaf = A.stride;
    __CLPK_integer nrhs = B.rows;
    __CLPK_integer ldb = B.stride;
    matrix X(B.rows, B.cols);
    __CLPK_integer ldx = X.stride;
    f_t ferr[nrhs], berr[nrhs];
    __CLPK_integer iwork[n];
    f_t work[3 * n];
    f_t s[n];
    f_t rcond;
    dposvx_(&fact, &uplo, &n, &nrhs, A.data, &lda, AF.data, &ldaf, &equed, s, B.data, &ldb, X.data, &ldx, &rcond, ferr, berr, work, iwork, &info);
    if(info) {
        fprintf(stderr, "solve: posvx failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    for(int i = 0; i < B.rows; ++i) {
        for(int j = 0; j < B.cols; ++j) {
            B(i, j) = X(i, j);
        }
    }
    
    return true;
}

bool matrix_solve_svd(matrix &A, matrix &B)
{
    __CLPK_integer info;
    f_t sv[A.rows];
    __CLPK_integer rank;
    f_t rcond = -1;
    __CLPK_integer lwork = -1;
    __CLPK_integer n = A.cols;
    __CLPK_integer lda = A.stride;
    __CLPK_integer nrhs = B.rows;
    __CLPK_integer ldb = B.stride;
    f_t work0;
    gelsd(&n, &n, &nrhs, A.data, &lda, B.data, &ldb, sv, &rcond, &rank, &work0, &lwork, 0, &info);
    lwork = (int)work0;
    f_t work[lwork];
    __CLPK_integer iwork[lwork];
    gelsd(&n, &n, &nrhs, A.data, &lda, B.data, &ldb, sv, &rcond, &rank, work, &lwork, iwork, &info);

    //fprintf(stderr, "svd reported rank: %ld\n", rank);
    if(info) {
        fprintf(stderr, "solve: sgelsd failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    return true;
}


//returns in new matrices, but destroys A
// S should have dimension at least max(1, min(m,n))
bool matrix_svd(matrix &A, matrix &U, matrix &S, matrix &Vt)
{
    __CLPK_integer info;

    __CLPK_integer lwork = -1;
    f_t work0;
    __CLPK_integer iwork[8 * A.cols];
    //gesvd/dd is fortran, so V^T and U are swapped
    __CLPK_integer n = A.rows;
    __CLPK_integer m = A.cols;
    __CLPK_integer lda = A.stride;
    __CLPK_integer ldVt = Vt.stride;
    __CLPK_integer ldU = U.stride;

    gesdd((char *)"A", &m, &n, A.data, &lda, S.data, Vt.data, &ldVt, U.data, &ldU, &work0, &lwork, iwork, &info);
    lwork = (int)work0;
    f_t work[lwork];
    gesdd((char *)"A", &m, &n, A.data, &lda, S.data, Vt.data, &ldVt, U.data, &ldU, work, &lwork, iwork, &info);
    if(info) {
        fprintf(stderr, "svd: gesvd failed: %ld\n", info);
        fprintf(stderr, "\n******ALERT -- THIS IS FAILURE!\n\n");
        return false;
    }
    return true;
}

void matrix_transpose(matrix &dst, const matrix &src)
{
    assert(dst.cols == src.rows && dst.rows == src.cols);
    for(int i = 0; i < dst.rows; ++i) {
        for(int j = 0; j < dst.cols; ++j) {
            dst(i, j) = src(j, i);
        }
    }
}

matrix &matrix_dereference(matrix *m)
{
    return *m;
}

//need to put this test around every operation that affects cov. (possibly with #defines, google test?)
bool test_posdef(const matrix &m)
{
    matrix tmp(m.rows, m.cols);
    bool ret = true;
    for(int i = 0; i < m.rows; ++i)
    {
        if(m(i, i) < 0.) {
            fprintf(stderr, "negative diagonal element: %d is %e\n", i, m(i, i));
            ret = false;
        }
        tmp(i, i) = m(i, i);
        for(int j = i + 1; j < m.cols; ++j)
        {
            if(m(i, j) != m(j, i)) {
                fprintf(stderr, "not symmetric: m(%d, %d) = %e; m(%d, %d) = %e\n", i, j, m(i, j), j, i, m(j, i));
                ret = false;
            }
            tmp(i, j) = tmp(j, i) = m(i, j);
        }
    }
    if(!ret) return false;
    return matrix_cholesky(tmp);
}
