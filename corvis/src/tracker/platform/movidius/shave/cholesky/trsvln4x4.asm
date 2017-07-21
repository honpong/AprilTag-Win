  .version 00.50.01

; Triangular lower solve vector - 4x4 kernel

  .code .text.trsvln4x4

; solves x from Lx = b (L being lower triangular)
; void trsvln4x4( float *x, const float *L, const float *b, int wL );

; x[0] = 1/L[0][0] *  b[0]
; x[1] = 1/L[1][1] *( b[1] - L[1][0]*x[0] )
; x[2] = 1/L[2][2] *( b[2] - L[2][0]*x[0] - L[2][1]*x[1] )
; x[3] = 1/L[3][3] *( b[3] - L[3][0]*x[0] - L[3][1]*x[1] - L[3][2]*x[2] )

  .lalign
  .type trsvln4x4, @function
trsvln4x4:
  ; input arguments
  .set  x i18
  .set  L i17
  .set  b i16
  .set wL i15
  ; locals
  .set l00 i14
  .set l10 i12
  .set n11 i13 ; 1/l11
  .set n22 i11 ; 1/l22
  .set n33 i10 ; 1/l33
  .set  L2 v0
  .set  L3 v1
  .set  b_ v2
  .set  x0 i6
  .set  x1 i7
  .set  x2 i8
  .set  x3 i9
  .set one i5 ; 1.0f
  lsu1.ldi.32  l00, L wL
  lsu1.ldxv     b_, b
  lsu0.ldil    one, 1.0f32
  lsu1.ldi.64  l10, L wL
  lsu1.ldxvi    L2, L wL
  lsu0.ldih    one, 1.0f32
  lsu1.ldxvi    L3, L wL
  nop
  lsu1.cp       x0, v2.0    || lsu0.cp   x1, v2.1 ; x1 = b1;   x0 = b0
  sau.div.f32   x0, x0 l00  || cmu.cpvi  x2, v2.2 ; x2 = b2;   x0 = b0 / l00
  sau.div.f32  n11, one n11 || lsu0.cp   x3, v2.3 ; x3 = b3;  n11 = 1 / n11
                               lsu0.cp   i1, v0.0 ; i1 = l20;
  lsu0.cp      n22, v0.2
  sau.div.f32  n22, one n22 || lsu0.cp   i2, v1.0 ; n22 = 1 / l22; i2 = l30
  lsu0.cp      n33, v1.3
  sau.div.f32  n33, one n33                       ; n33 = 1 / l33
  nop 5
  ; here: x0 = b0/l00
  sau.mul.f32   i0, x0 l10                        ; i0 = l10 * x0
  ; here: n11 = 1/n11
  nop ; can't do sau.xxx here
  sau.mul.f32   i1, x0 i1                         ; i1 = l20 * x0
  nop ; can't do sau.xxx here
  ; here: n22 = 1/l22
  sau.sub.f32   x1, x1 i0                         ; x1 = b1 - l10*x0
  sau.sub.f32   x2, x2 i1                         ; x2 = b2 - l20*x0
  ; here: n33 = 1/l33
  sau.mul.f32   i2, x0 i2                         ; i2 = l30*x0
  sau.mul.f32   x1, x1 n11  || lsu0.cp   i0, v0.1 ; x1 =(b1 - l00*x0)/ l11; i0 = l21
  nop
  sau.sub.f32   x3, x3 i2   || lsu0.cp   i1, v1.1 ; x3 = b3 - l30*x0;       i1 = l31
  sau.mul.f32   i0, i0 x1   || lsu1.sti.64 x0, x  ; i0 = l21 * x1
  sau.mul.f32   i1, i1 x1                         ; i1 = l31 * x1
  nop
  sau.sub.f32   x2, x2 i0                         ; x2 = b2 - l20*x0 - l21*x1
  sau.sub.f32   x3, x3 i1                         ; x3 = b3 - l30*x0 - l31*x1
  nop
  sau.mul.f32   x2, x2 n22  || lsu0.cp   i0, v1.2 ; x2 =( b2 - l20*x0 - l21*x1 )/ l22; i0 = l32
  nop 2
  sau.mul.f32   i0, i0 x2                         ; i0 = l32 * x2
  nop 2
  sau.sub.f32   x3, x3 i0   || bru.jmp  i30       ; x3 = b3 - l30*x0 - l31*x1 - l32*x2
    nop 2
    sau.mul.f32 x3, x3 n33                        ; x3 =( b3 - l30*x0 - l31*x1 - l32*x2 )/ l33
    nop 2
    lsu1.st.64  x2, x
  .size trsvln4x4, . - trsvln4x4

  .end
