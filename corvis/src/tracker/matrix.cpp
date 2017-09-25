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

#include <Eigen/Cholesky>

#include <stdio.h>
#include "Trace.h"

#ifdef MYRIAD2
#include "platform_defines.h"
#endif

#if defined(ENABLE_BLIS_GEMM) || defined(ENABLE_SHAVE_SOLVE) || defined(HAVE_BLIS)
#include "blis.h"
#ifdef MYRIAD2
#include "blis_defines.h"
#include <rtems.h>
#else
static struct blis_initialize { blis_initialize() { bli_init(); } } blis_initialize;
#endif
#include <mutex>
static std::mutex blis_mutex;

static void blis_set_object(const matrix &m, obj_t *obj, bool trans = false)
{
    if (trans) bli_obj_create_with_attached_buffer(BLIS_FLOAT, m.cols(), m.rows(), (void *)&m(0,0), 1, m.get_stride(), obj);
    else       bli_obj_create_with_attached_buffer(BLIS_FLOAT, m.rows(), m.cols(), (void *)&m(0,0), m.get_stride(), 1, obj);
}

#endif

#if defined(ENABLE_BLIS_GEMM) || defined(HAVE_BLIS)
static void matrix_product_blis(matrix &res, const matrix &A, const matrix &B, bool transA, bool transB, const float dst_scale, const float scale)
{
    obj_t Aobj, Bobj, resObj, scaleObj, dstScaleObj;

    blis_set_object(A, &Aobj, transA);
    blis_set_object(B, &Bobj, transB);
    blis_set_object(res, &resObj);
    bli_obj_scalar_init_detached(BLIS_FLOAT, &scaleObj);
    bli_obj_scalar_init_detached(BLIS_FLOAT, &dstScaleObj);
    bli_setsc(scale, 0.0, &scaleObj);
    bli_setsc(dst_scale, 0.0, &dstScaleObj);

    blis_mutex.lock();
    bli_gemm(&scaleObj, &Aobj, &Bobj, &dstScaleObj, &resObj);
    blis_mutex.unlock();
#ifdef MYRIAD2
    rtems_cache_invalidate_multiple_data_lines( (void *)res.Data(), res.rows() * res.get_stride() * sizeof(f_t) );
#endif
}
#endif // ENABLE_BLIS_GEMM

#if defined(ENABLE_SHAVE_SOLVE) || defined(HAVE_BLIS)
static void matrix_solve_blis(const matrix &A, const matrix &B, uplo_t uplo, bool transB)
{
    obj_t Aobj, Bobj;
    blis_set_object(A, &Aobj, false);
    bli_obj_set_uplo(uplo, Aobj);
    bli_obj_set_struc(BLIS_TRIANGULAR, Aobj);
    blis_set_object(B, &Bobj, transB);

    blis_mutex.lock();
    bli_trsm(BLIS_RIGHT, &BLIS_ONE, &Aobj, &Bobj);
    blis_mutex.unlock();
#ifdef MYRIAD2
    rtems_cache_invalidate_multiple_data_lines( (void *)B.Data(), B.rows() * B.get_stride() * sizeof(f_t) );
#endif
}
#endif // ENABLE_SHAVE_SOLVE

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

    START_EVENT(SF_GEMM, 0);

#ifdef ENABLE_BLIS_GEMM
    int k = trans1 ? A.rows() : A.cols();
    if (
        (k > 3) &&
        (A.get_stride() % 16 == 0) &&
        (B.get_stride() % 16 == 0) &&
        (res.get_stride() % 16 == 0)
      )
    {
        START_EVENT(SF_GEMM_HW, 0);
        matrix_product_blis(res, A, B, trans1, trans2, dst_scale, scale);
        END_EVENT(SF_GEMM_HW, k);
    }
    else
#endif
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
    END_EVENT(SF_GEMM, 0);
}

f_t matrix_check_condition(matrix &A)
{
    return A.map().llt().rcond();
}

bool matrix_half_solve(matrix &A, matrix &B) // A = L L^T; B = B L^-T
{
    START_EVENT(SF_MSOLVE, 0);
    {
    matrix::Map
        A_map { &A(0,0), A.rows(), A.cols(), A.get_stride() },
        B_map { &B(0,0), B.rows(), B.cols(), B.get_stride() };
    Eigen::LLT< Eigen::Ref<decltype(A_map)>, Eigen::Upper > llt(A_map);
    if (llt.info() == Eigen::NumericalIssue)
        return false;
#if defined(ENABLE_SHAVE_SOLVE) || defined(HAVE_BLIS)
    if (A.rows() > 3)
        matrix_solve_blis(A, B, BLIS_UPPER, false);
    else
#endif
    llt.matrixL().solveInPlace(B_map.transpose());
    }
    END_EVENT(SF_MSOLVE, 0);
    return true;
}

//need to put this test around every operation that affects cov. (possibly with #defines, google test?)
bool test_posdef(const matrix &m)
{
    f_t norm = (m.map() - m.map().transpose()).norm();
    if(norm > F_T_EPS)
        return false;
    return m.map().llt().info() != Eigen::NumericalIssue;
}
