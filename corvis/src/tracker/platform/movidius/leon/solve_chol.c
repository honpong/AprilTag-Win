#include "solve_chol.h"

#include <stdio.h>
#include <string.h>
#include <DrvSvu.h>
#include <DrvTimer.h>
#include "rtems.h"

#define FIRST_SHAVE 7
#define MAX_SHAVES 4
#define SHAVES_USED 4

// functions from shaves
extern void *(cholesky7__exit);
extern void *(cholesky8__exit);
extern void *(cholesky9__exit);
extern void *(cholesky10__exit);

extern void *(cholesky7_trsvl_lnlt);
extern void *(cholesky8_trsvl_lnlt);
extern void *(cholesky9_trsvl_lnlt);
extern void *(cholesky10_trsvl_lnlt);

extern void *(cholesky7_potrf_ln);

void *f__exit[MAX_SHAVES] = {
    &cholesky7__exit,
    &cholesky8__exit,
    &cholesky9__exit,
    &cholesky10__exit,
};
void* f_trsvl_lnlt[MAX_SHAVES] = {
    &cholesky7_trsvl_lnlt,
    &cholesky8_trsvl_lnlt,
    &cholesky9_trsvl_lnlt,
    &cholesky10_trsvl_lnlt,
};

void *f_potrf_ln[1] = {
    &cholesky7_potrf_ln,
};


#define invd_cache() \
    { DrvLL2CFlushOpOnAllLines(LL2C_OPERATION_INVALIDATE,0); asm volatile( "flush"); }

#define MAX_MATRIX 36*1024
__attribute__((section(".cmx_direct.data"))) float cmxA[MAX_MATRIX];

void solve_chol(float * A, float * Bt, float * X, int rows, int Bt_rows, int A_stride, int Bt_stride, int X_stride)
{
    for (int i = 0; i < SHAVES_USED; i++) {
        swcResetShave( FIRST_SHAVE + i );
        swcSetAbsoluteDefaultStack( FIRST_SHAVE + i );
    }

	//make sure A and B are in ddr - not needed when cache in write through
	 //invd_cache();
    // Phase 1 - execute on single shave only
    void **potrf = f_potrf_ln;
    DrvSvutIrfWrite( FIRST_SHAVE, 30, (u32)f__exit[0] );
    swcStartShaveCC( FIRST_SHAVE, (u32)potrf[0], "iiii", A, cmxA, rows, A_stride*sizeof(float) );
    swcWaitShave( FIRST_SHAVE);

    int n = rows;
    int cols = Bt_rows;

    //SHAVES_USED must be multiple of 4 or need to treat excessive rows
    int parallel_cols = (int)(cols / SHAVES_USED) * SHAVES_USED;

    for(int i = 0; i < parallel_cols; i += SHAVES_USED) {
    	 for(int t = 0; t < SHAVES_USED; t++) {
			DrvSvutIrfWrite( FIRST_SHAVE + t, 30, (u32)f__exit[t] );
			swcStartShaveCC( FIRST_SHAVE + t, (u32)f_trsvl_lnlt[t], "iiiiii", X+(i + t)*X_stride, cmxA, Bt+(i + t)*Bt_stride, n, n*sizeof(float),Bt_rows);
		}

		for (int t = 0; t < SHAVES_USED; t++) {
			swcWaitShave( FIRST_SHAVE + t );
		}
    }
}

