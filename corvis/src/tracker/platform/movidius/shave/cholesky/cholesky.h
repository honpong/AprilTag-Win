#ifndef SHAVE_CHOLESKY_H
#define SHAVE_CHOLESKY_H

// NOTE: the entry point declaration convention apparently changed from Jan'16
// (no idea to what), so I'll just comment this out, and make sure that the
// on-call return address points to _exit() - also added - instead

// #define ENTRYPOINT __attribute__((dllexport))
#define ENTRYPOINT

// width are in bytes for all the kernels

//-----------------------------------------------------------------------------

// transposed matrix by vector multiplication - compute 4x1 cell
void gemvltNx4( float *c, const float *A, const float *b, int n, int wA );

//-----------------------------------------------------------------------------

// symmetric rank k update - compute 4x4 cell
void syrktnNx4( float* C, const float *A, const float *B,
                int n, int wC, int wBA );

// symmetric rank k update - reverse copy row
void rcpy4xN( float *B, const float *A, int n, int wB, int wA );

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

// triangular solve lower kernel
void trsvln4x4( float *x, const float *L, const float *b, int wL );

// triangular solve normal, update kernel
void trsvn4x4_U( float *b, const float *A, const float *x,int wA );

//-----------------------------------------------------------------------------

// triangular solve lower transposed kernel
void trsvlt4x4( float *x, const float *L, const float *b, int wL );

// triangular solve transposed, update kernel
void trsvt4x4_U( float *b, const float *A, const float *x,int wA );


#endif
