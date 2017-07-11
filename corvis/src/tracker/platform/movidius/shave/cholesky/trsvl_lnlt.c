#include "cholesky.h"
#include "swcCdma.h"
#include <svuCommonShave.h>
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
            float* __restrict__ b, int n, int sL)
{

	//dma transaction of b to cmxB
	dmaTransactionList_t dmaTask;
	dmaTransactionList_t* dmaRef;
	u32 dmaRequsterId = dmaInitRequester(1);
	dmaRef = dmaCreateTransaction(dmaRequsterId, &dmaTask, (u8*)(b),
			(u8*) cmxB, n * sizeof(float));
	dmaStartListTask(dmaRef);
	dmaWaitTask(dmaRef);

	trsvl_ln(cmxY, L_, cmxB, n, sL);
    trsvl_lt(cmxY, L_, cmxY, n, sL);

    //out DMA to y
	dmaRef = dmaCreateTransaction(dmaRequsterId, &dmaTask, (u8*)(cmxY),
				(u8*) y, n * sizeof(float));
	dmaStartListTask(dmaRef);
	dmaWaitTask(dmaRef);

	SHAVE_HALT;
}
#ifdef __cplusplus
}
#endif
