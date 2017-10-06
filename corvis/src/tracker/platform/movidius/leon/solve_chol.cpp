#include <mv_types.h>
#include "solve_chol.h"
#include "Shave.h"

#define FIRST_CHOLESKY_SHAVE 0
#define SOLVE_SHAVES 4
#define MAX_MATRIX 36*1024

extern void *(cholesky0_trsvl_lnlt);
extern void *(cholesky1_trsvl_lnlt);
extern void *(cholesky2_trsvl_lnlt);
extern void *(cholesky3_trsvl_lnlt);

extern void *(cholesky0_potrf_ln);

void* f_trsvl_lnlt[SOLVE_SHAVES] = {
    &cholesky0_trsvl_lnlt,
    &cholesky1_trsvl_lnlt,
    &cholesky2_trsvl_lnlt,
    &cholesky3_trsvl_lnlt,
};

void *f_potrf_ln[1] = {
    &cholesky0_potrf_ln,
};

__attribute__((section(".cmx_direct.data"))) float cmxA[MAX_MATRIX];
Shave* shaves[SOLVE_SHAVES] = { NULL };

void solve_chol(float * A, float * Bt, float * X, int rows, int Bt_rows, int A_stride, int Bt_stride, int X_stride)
{
    for (int i = 0; i < SOLVE_SHAVES; i++) {
        shaves[i] = Shave::get_handle(i + FIRST_CHOLESKY_SHAVE);
    }
    // Phase 1 - execute on single shave only
    void **potrf = f_potrf_ln;
    shaves[0]->start( (u32)potrf[0], "iiii", A, cmxA, rows, A_stride*sizeof(float) );
    shaves[0]->wait();

    int parallel_cols = (int)(Bt_rows / SOLVE_SHAVES) * SOLVE_SHAVES;
    int startIndex[SOLVE_SHAVES + 1] = {0};

    for(int i = 0; i < SOLVE_SHAVES; ++i){
        startIndex[i + 1] = startIndex[i] + parallel_cols;
        startIndex[i + 1] = (startIndex[i + 1] > Bt_rows)? Bt_rows : startIndex[i + 1];
    }

    for(int i = 0; i < SOLVE_SHAVES; ++i){
        shaves[i]->start( (u32)f_trsvl_lnlt[i],
                          "iiiiiiii",
                          X + startIndex[i] * X_stride,
                          cmxA,
                          Bt + startIndex[i] * Bt_stride,
                          rows,
                          X_stride,
                          rows,
                          Bt_stride,
                          startIndex[i + 1] - startIndex[i]);

        shaves[i]->wait();
    }
}
