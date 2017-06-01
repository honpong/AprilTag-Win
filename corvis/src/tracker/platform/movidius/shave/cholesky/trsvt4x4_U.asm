  .version 00.50.01

; Triangular solve vector - transposed 4x1 free coefficients update

; b[i] -= sum:j( A[j][i]*x[j] );

  .code .text.trsvt4x4_U

  .lalign
  .type trsvt4x4_U, @function
trsvt4x4_U:
  ; input arguments
  .set b_ i18
  .set  A i17
  .set  x i16
  .set  w i15
  ; locals
  .set  A0 v0
  .set  A1 v1
  .set  A2 v2
  .set  A3 v3
  .set  x0 v4
  .set  x1 v5
  .set  x2 v6
  .set  x3 v7
  .set   b v8
  lsu1.ldxvi     A0, A w   || lsu0.ldxv      b, b_
  nop
  lsu1.ldxvi     A1, A w   || lsu0.ldi.32r  x0, x
  nop
  lsu1.ldxvi     A2, A w   || lsu0.ldi.32r  x1, x
  nop
  lsu1.ldxv      A3, A
  nop
  vau.accpz.f32      b     || lsu1.ldi.32r  x2, x
  vau.macn.f32       A0 x0 || lsu1.ld.32r   x3, x
  nop 2
  vau.macn.f32       A1 x1
  nop 2
  vau.macn.f32       A2 x2
  nop 2
  vau.macnw.f32   b, A3 x3 || bru.jmp        i30
    nop 4
    lsu1.stxv     b, b_
    nop

  .size trsvt4x4_U, . - trsvt4x4_U

  .end
