  .version 00.50.01

; Triangular solve vector - normal 4x1 free coefficients update

; b[i] -= sum:j( A[i][j]*x[j] );

  .code .text.trsvn4x4_U

  .lalign
  .type trsvn4x4_U, @function
trsvn4x4_U:
  ; input arguments
  .set  b i18
  .set  A i17
  .set x_ i16
  .set  w i15
  ; locals
  .set  b0 i13
  .set  b1 i14
  .set  b2 i11
  .set  b3 i12
  .set  A0 v0
  .set  A1 v1
  .set  A2 v2
  .set  A3 v3
  .set   x v4
  lsu1.ldxvi     A0, A w   || lsu0.ldxv      x, x_
  nop
  lsu1.ldxvi     A1, A w
  nop
  lsu1.ldxvi     A2, A w
  nop
  lsu1.ldxv      A3, A
  nop
  vau.mul.f32    A0, A0 x  || lsu1.ldi.64   b0, b
  nop
  vau.mul.f32    A1, A1 x  || lsu1.ld.64    b2, b
  sau.sumx.f32   i0, A0
  vau.mul.f32    A2, A2 x  || iau.incs       b, -8
  sau.sumx.f32   i1, A1
  vau.mul.f32    A3, A3 x
  sau.sumx.f32   i2, A2
  sau.sub.f32    b0, b0 i0
  sau.sumx.f32   i3, A3
  sau.sub.f32    b1, b1 i1
  bru.jmp       i30
    sau.sub.f32  b2, b2 i2
    lsu1.sti.64  b0, b
    sau.sub.f32  b3, b3 i3
    nop 2
    lsu1.sti.64  b2, b

  .size trsvn4x4_U, . - trsvn4x4_U

  .end
