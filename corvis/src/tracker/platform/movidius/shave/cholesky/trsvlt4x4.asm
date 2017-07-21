  .version 00.50.01

; Triangular solve vector - 4x4 kernel

  .code .text.trsvlt4x4
        
; solves x from (L^T)*x = b (L being lower triangular)
; void trsvlt4x4( float *x, const float *L, const float *b, int w );

; x[3] = 1/L[3][3] *  b[3]
; x[2] = 1/L[2][2] *( b[2] - L[3][2]*x[3] )
; x[1] = 1/L[1][1] *( b[1] - L[3][1]*x[3] - L[2][1]*x[2] )
; x[0] = 1/L[0][0] *( b[0] - L[3][0]*x[3] - L[2][0]*x[2] - L[1][0]*x[1] )

  .lalign
  .type trsvlt4x4, @function
trsvlt4x4:
  ; input arguments
  .set  x i18
  .set  L i17
  .set  b i16
  .set  w i15
  ; locals
  .set n00 i14 ; 1/l00
  .set l10 i12
  .set n11 i13 ; 1/l11
  .set n22 i11 ; 1/l22
  .set l33 i10 ; l33
  .set  L2 v0
  .set  L3 v1
  .set  b_ v2
  .set  x2 i8
  .set  x3 i9
  .set  x0 i6
  .set  x1 i7
  .set one i5 ; 1.0f
  lsu1.ldi.32  n00, L w
  lsu1.ldi.64  l10, L w     || lsu0.ldil one, 1.0f32
  lsu1.ldxvi    L2, L w     || lsu0.ldih one, 1.0f32
  lsu0.ldxv     b_, b
  lsu1.ldxv     L3, L
  nop 3
  sau.div.f32  n00, one n00
  sau.div.f32  n11, one n11
  lsu0.cp      n22, v0.2    || lsu1.cp    x1, v2.1 ; x1 = b1
  sau.div.f32  n22, one n22 || lsu0.cp    x3, v2.3 ; x3 = b3
  lsu0.cp      l33, v1.3    || lsu1.cp    x2, v2.2 ; x2 = b2
  sau.div.f32   x3, x3 l33  || lsu0.cp    x0, v2.0 ; x0 = b0
  lsu0.cp       i0, v1.2    || lsu1.cp    i1, v1.1 ; i0 = l32; i1 = l31
  lsu0.cp       i2, v0.1    || lsu1.cp    i3, v1.0 ; i2 = l21; i3 = l30
  lsu0.cp       i4, v0.0                           ; i4 = l20
  nop 8
  ; here: x3 = b3/l33
  sau.mul.f32   i0, x3 i0   ; i0 = l32*x3
  sau.mul.f32   i1, x3 i1   ; i1 = l31*x3
  sau.mul.f32   i3, x3 i3   ; i3 = l30*x3
  sau.sub.f32   x2, x2 i0   ; x2 = b2 - l32*x3
  sau.sub.f32   x1, x1 i1   ; x1 = b1 - l31*x3
  sau.sub.f32   x0, x0 i3   ; x0 = b0 - l30*x3
  sau.mul.f32   x2, x2 n22  ; x2 =( b2 - l32*x3 )/ l22
  nop 2
  ; here: x2 =( b2 - l32*x3 )/ u22
  sau.mul.f32   i2, x2 i2   || lsu1.sto.64   x2, x 8 ; i2 = l21*x2
  sau.mul.f32   i4, x2 i4                            ; i4 = l20*x2
  nop
  sau.sub.f32   x1, x1 i2   ; x1 = b1 - l31*x3 - l21*x2
  sau.sub.f32   x0, x0 i4   ; x0 = b0 - l30*x3 - l20*x2
  nop
  sau.mul.f32   x1, x1 n11  ; x1 =( b1 - l31*x3 - l21*x2 )/ l11
  nop 2
  ; here: x1 =( b1 - l31*x3 - l21*x2 )/ l11
  sau.mul.f32  l10, x1 l10
  nop 2
  sau.sub.f32   x0, x0 l10  || bru.jmp i30           ; x0 = b0 - l30*x3 - l20*x2 - l10*x1
    nop 2
    sau.mul.f32 x0, x0 n00  ; x0 =( b0 - l30*x3 - l20*x2 - l10*x1 )/ l00
    nop 2
    lsu1.st.64  x0, x
  .size trsvlt4x4, . - trsvlt4x4

  .end
