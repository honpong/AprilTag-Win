/* Triangular solve vector [trsv    ]
   left-side coefficients  [    l   ]
   lower triangular        [     _l ]
   normal                  [       n] */

// perform y = b*(L^-1) <=> solve y from Ly=b
//   in L: L[n][n] #LT
//   in b: b[n]
//  out y: y[n]

#include "cholesky.h"

void ENTRYPOINT
trsvl_ln( float* __restrict__ y, const float* __restrict__ L_, 
          float* __restrict__ b, int n, int sL )
{
  const int nL = sL / sizeof(float);
# define L(i,j) L_[(i)*nL+(j)]
  int i;
  for( i = 0; i < n; i += 4 ) {
    trsvln4x4( &y[i], &L(i,i), &b[i], sL );
    int j;
    for( j = i + 4; j < n; j += 4 )
      trsvn4x4_U( &b[j], &L(j,i), &y[i], sL );
  }
# undef L
}
