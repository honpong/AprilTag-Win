/* Cholesky LL' decomposition [potrf   ]
   lower triangular, normal   [     _ln] */

// potrf, lower triangular output, normal (i.e. not transposed)
// L@out = A | A * A' == L@in
void potrfln4x4( float *L, int w );

// potrf update operator
//   C = ( C - sum[k=0:n:4]{ A@(0,k) * (B@(0,k))' } ) / D'
//   E -= C * C'
// with
//   rectangular C[4][4], A[4][n], B[4][n]
//   lower triangular D[4][4], E[4][4]
void potrfln4x4upd( float *C, float *E,
                    const float *A, const float *B, int n,
                    const float *D,
                    int wCEA, int wBD );

// perform L = A | AA'=L
//   in L: L[n][n] #SPD
//  out L: L[n][n] #LT
__attribute__((dllexport))
void potrf_ln(float *L_, int n, int sL)
{
    const int nL = sL / sizeof(float);
# define L(i,j) L_[(i)*nL+(j)]
    for( int i = 0; i < n; i += 4 ) {
        potrfln4x4( &L(i,i), sL );
        for( int j = i + 4; j < n; j += 4 ) {
            // L@j,i =( L@j,i - sum[k=0:i:4]{ L@j,k * (L@i,k)' } ) / (L@i,i)'
            // L@j,j -= L@j,i * (L@j,i)'
            potrfln4x4upd( &L(j,i), &L(j,j), &L(j,0), &L(i,0), i, &L(i,i), sL, sL );
        }
    }
# undef L
}
