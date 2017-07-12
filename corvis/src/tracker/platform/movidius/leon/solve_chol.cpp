#include <mv_types.h>
#include "solve_chol.h"
#include "Shave.h"

#define FIRST_CHOLESKY_SHAVE 0
#define SOLVE_SHAVES 4
#define MAX_MATRIX 36*1024

extern void *(cvrt0_trsvl_lnlt);
extern void *(cvrt1_trsvl_lnlt);
extern void *(cvrt2_trsvl_lnlt);
extern void *(cvrt3_trsvl_lnlt);

extern void *(cvrt0_potrf_ln);

void* f_trsvl_lnlt[SOLVE_SHAVES] = {
    &cvrt0_trsvl_lnlt,
    &cvrt1_trsvl_lnlt,
    &cvrt2_trsvl_lnlt,
    &cvrt3_trsvl_lnlt,
};

void *f_potrf_ln[1] = {
    &cvrt0_potrf_ln,
};

__attribute__((section(".cmx_direct.data"))) float cmxA[MAX_MATRIX];
Shave* shaves[SOLVE_SHAVES] = { NULL };

void solve_chol(float * A, float * Bt, float * X, int rows, int Bt_rows, int A_stride, int Bt_stride, int X_stride)
{
    for (int i = 0; i < SOLVE_SHAVES; i++) {
        shaves[i] = Shave::get_handle(i + FIRST_CHOLESKY_SHAVE);
        shaves[i]->acquire();
    }

    //make sure A and B are in ddr - not needed when cache in write through
     //invd_cache();
    // Phase 1 - execute on single shave only
    void **potrf = f_potrf_ln;
    shaves[0]->start( (u32)potrf[0], "iiii", A, cmxA, rows, A_stride*sizeof(float) );
    shaves[0]->wait();

    int n = rows;
    int cols = Bt_rows;

    //SHAVES_USED must be multiple of 4 or need to treat excessive rows
    int parallel_cols = (int)(cols / SOLVE_SHAVES) * SOLVE_SHAVES;

    for(int i = 0; i < parallel_cols; i += SOLVE_SHAVES) {
        for(int t = 0; t < SOLVE_SHAVES; t++) {
            shaves[t]->start( (u32)f_trsvl_lnlt[t], "iiiiii", X+(i + t)*X_stride, cmxA, Bt+(i + t)*Bt_stride, n, n*sizeof(float),Bt_rows);
        }

        for (int t = 0; t < SOLVE_SHAVES; t++) {
            shaves[t]->wait();
        }
    }

    for (int i = 0; i < SOLVE_SHAVES; i++) {
        shaves[i]->release();
    }

}
