/* Cholesky LL' decomposition [potrf   ]
   lower triangular, normal   [     _ln] */

// perform L = A | AA'=L
//   in L: L[n][n] #SPD
//  out L: L[n][n] #LT

#include "cholesky.h"
#include "swcCdma.h"

void ENTRYPOINT
potrf_ln( float* __restrict__ L_,float* __restrict__ cmxA, int n, int sL )
{

    //dma transaction of A to cmxA
    dmaTransactionList_t dmaTask;
    dmaTransactionList_t* dmaRef;
    u32 dmaRequsterId = dmaInitRequester(1);

    //strided DMA
    dmaRef = dmaCreateTransactionSrcStride(
                    dmaRequsterId,
                    &dmaTask,
                    (u8*)(L_),
                    (u8*)cmxA,
                    n * n * sizeof(float),
                    n * sizeof(float),         //src width
                    sL);                       //src stride

    dmaStartListTask(dmaRef);
    dmaWaitTask(dmaRef);
    sL = n * sizeof(float);
    const int nL = sL / sizeof(float);

# define L(i,j) cmxA[(i)*nL+(j)]
    int i;
    // foreach row
    for( i = 0; i < n; i += 4 ) {
        potrfln4x4( &L(i,i), sL );
        int j;
        for( j = i + 4; j < n; j += 4 ) {
            // L@j,i =( L@j,i - sum[k=0:i:4]{ L@j,k * (L@i,k)' } ) / (L@i,i)'
            // L@j,j -= L@j,i * (L@j,i)'
            potrfln4x4upd( &L(j,i), &L(j,j), &L(j,0), &L(i,0), i, &L(i,i), sL, sL );
        }
    }
# undef L
}
