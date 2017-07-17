#include "cholesky.h"
#include "swcCdma.h"
#include <svuCommonShave.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
static float cmxB[512];
static float cmxY[512];

void 
trsvl_ln( float* __restrict__ y, const float* __restrict__ L_,
          float* __restrict__ b, int n, int sL );
void 
trsvl_lt( float* __restrict__ y, const float* __restrict__ L_,
          float* __restrict__ b, int n, int sL );

void ENTRYPOINT
trsvl_lnlt( float* __restrict__ y, const float* __restrict__ L_,
            float* __restrict__ b, int n, int y_stride, int l_stride,
            int b_stride, int lines_number)
{
    float* y_line = y;
    float* b_line = b;

    //TODO:doron investigate why DMA ouside the loop hangs
    for(int i = 0; i < lines_number; ++i){
        memcpy((void*)cmxB, (void*)b_line, n * sizeof(float));
        trsvl_ln(cmxY, L_, cmxB, n, l_stride * sizeof(float));
        trsvl_lt(cmxY, L_, cmxY, n, l_stride * sizeof(float));
        memcpy((void*)y_line, cmxY, n * sizeof(float));
        y_line += y_stride;
        b_line += b_stride;
    }

    SHAVE_HALT;
}
#ifdef __cplusplus
}
#endif
