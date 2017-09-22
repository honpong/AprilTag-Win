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

#include <mutex>
#include <OsDrvShaveL2Cache.h>

#ifdef ENABLE_SHAVE_CHOLESKY

#include "solve_chol.h"
#define SHAVE_CHOLESKY_L2_PARTITION 0
static std::mutex cholesky_mutex;
#endif

#ifdef ENABLE_BLIS_GEMM
#include <DrvLeonL2C.h>
#include "blis.h"
#include "blis_defines.h"
#ifdef __RTEMS__
#include <rtems.h>
#endif
static std::mutex blis_mutex;

static void blis_set_object(const matrix &m, obj_t *obj, bool trans = false)
{
    if (trans) bli_obj_create_with_attached_buffer(BLIS_FLOAT, m.cols(), m.rows(), (void *)&m(0,0), 1, m.get_stride(), obj);
    else       bli_obj_create_with_attached_buffer(BLIS_FLOAT, m.rows(), m.cols(), (void *)&m(0,0), m.get_stride(), 1, obj);
}

void matrix_product_blis(matrix &res, const matrix &A, const matrix &B, bool transA, bool transB, const float dst_scale, const float scale)
{
    obj_t Aobj, Bobj, resObj, scaleObj, dstScaleObj;

    blis_set_object(A, &Aobj, transA);
    blis_set_object(B, &Bobj, transB);
    blis_set_object(res, &resObj);
    bli_obj_scalar_init_detached(BLIS_FLOAT, &scaleObj);
    bli_obj_scalar_init_detached(BLIS_FLOAT, &dstScaleObj);
    bli_setsc(scale, 0.0, &scaleObj);
    bli_setsc(dst_scale, 0.0, &dstScaleObj);
    bli_gemm(&scaleObj, &Aobj, &Bobj, &dstScaleObj, &resObj);

    rtems_cache_invalidate_multiple_data_lines( (void *)res.Data(), res.rows() * res.get_stride() * sizeof(f_t) );
}

#endif // ENABLE_BLIS_GEMM
#ifdef ENABLE_SHAVE_CHOLESKY
bool matrix_cholesky_shave(matrix &A, matrix &B)
{
    // Fast path always has this size(3,24), don't run shave cholesky
    // here (use Eigen instead) as a workaround to avoid an unknown
    // issue which causes us to lock waiting on a DMA transaction on
    // the fast path
    if(A.rows() == 3 && B.rows() <= 25) return false;
    START_EVENT(SF_MSOLVE_HW, 0);

	int N_orig = A.rows();
	int M_orig = B.rows();
	int N = N_orig;
	int M = M_orig;
	int N_pad = 4 - (N % 4);
	int M_pad = 4 - (M % 4);
	//check that we can resize B
	//no need to check A, because it is ensured before calling solve
	bool resize = ((N_pad == 4 || N + N_pad <= B.get_stride()) &&
			(M_pad == 4 || M + M_pad <= B.Maxrows()));
   if (resize )  {
	    // Pad A and B to be % 4 = 0
        // A should be NxN
        // B should be MxN (because we operate on transposed)
        if(N_pad != 4) {
            N += N_pad;
            A.resize(N,N);
            // 0 the padded rows and cols
            for(int row = 0; row < N; row++)
                for(int col = N_orig; col < N; col++)
                    A(row,col) = 0;
            for(int row = N_orig; row < N; row++)
                for(int col = 0; col < N; col++)
                    A(row,col) = 0;
            // set the diagonal to 1
            for(int diag = N_orig; diag < N; diag++)
                A(diag, diag) = 1;

            // 0 the padded cols of B
            B.resize(M,N);
            for(int row = 0; row < M; row++)
                for(int col = N_orig; col < N; col++)
                    B(row,col) = 0;
        }

        if(M_pad != 4) {
            // 0 pad the rows of B
            M += M_pad;
            B.resize(M,N);
            for(int row = M_orig; row < M; row++)
                for(int col = 0; col < N; col++)
                    B(row,col) = 0;
        }
        cholesky_mutex.lock();
        solve_chol(A.Data(), B.Data(), B.Data(), N, M, A.get_stride(), B.get_stride(), B.get_stride());
        cholesky_mutex.unlock();
        //invalidate cache B range
        /*int sc = OS_MYR_DRV_SUCCESS;
          if ((sc = OsDrvShaveL2CachePartitionFlush(SHAVE_CHOLESKY_L2_PARTITION,
               PERFORM_INVALIDATION)) != OS_MYR_DRV_SUCCESS)
               printf("ERROR: OsDrvShaveL2CachePartitionFlush %lu\n", sc);*/
       
        rtems_cache_invalidate_multiple_data_lines( (void *)B.Data(), M * B.get_stride() * sizeof(f_t) );
        
        A.resize(N_orig, N_orig);
        B.resize(M_orig, N_orig);
    }
    END_EVENT(SF_MSOLVE_HW, 0);
   return resize;
}
#endif
#endif //MYRIAD2

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
    	blis_mutex.lock();
        START_EVENT(SF_GEMM_HW, 0);
        matrix_product_blis(res, A, B, trans1, trans2, dst_scale, scale);
        END_EVENT(SF_GEMM_HW, k);
        blis_mutex.unlock();
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
#ifdef ENABLE_SHAVE_CHOLESKY
    if (!matrix_cholesky_shave(A, B))
#endif
    {
    matrix::Map
        A_map { &A(0,0), A.rows(), A.cols(), A.get_stride() },
        B_map { &B(0,0), B.rows(), B.cols(), B.get_stride() };
    Eigen::LLT< Eigen::Ref<decltype(A_map)> > llt(A_map);
    if (llt.info() == Eigen::NumericalIssue)
        return false;
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
