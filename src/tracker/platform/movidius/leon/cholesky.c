#include "Shave.h"
#include "cholesky.h"
#include <rtems/rtems/cache.h>

extern void cholesky0_potrf_ln();

void potrf_ln(float *L, int n, int sL) {
    shave_start(0, (u32)cholesky0_potrf_ln, "iii", L, n, sL);
    shave_wait(0);
    rtems_cache_invalidate_multiple_data_lines( L, n * sL );
}

