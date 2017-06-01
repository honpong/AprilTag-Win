/* Triangular solve vector [trsv    ]
   left-side coefficients  [    l   ]
   lower triangular        [     _l ]
   transposed              [       t] */

// perform x = y*(L'^-1) <=> solve x from L'x=y
//   in L: L[n][n] #LT
//   in y: y[n]
//  out x: x[n]

#include "cholesky.h"

void ENTRYPOINT
trsvl_lt( float* __restrict__ x, const float* __restrict__ L_, 
          float* __restrict__ y, int n, int sL )
{
  const int nL = sL / sizeof(float);
# define L(i,j) L_[(i)*nL+(j)]
  int i;
  for( i = n-4; i >= 0; i -= 4 ) {
    trsvlt4x4( &x[i], &L(i,i), &y[i], sL );
    int j;
    for( j = 0; j < i; j += 4 )
      trsvt4x4_U( &y[j], &L(i,j), &x[i], sL );
  }
# undef L
}
