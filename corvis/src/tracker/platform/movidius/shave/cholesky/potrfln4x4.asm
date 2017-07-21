  .version 00.50.01

; Cholesky LL factorization - 4x4 kernel
; instructions: 266 (incl the sqrts: 4x47, so LL_chol4x4 itself is 78)
; model fpops: n_add=10 n_mul=16 n_div=3 n_sqrt=4
;  sqrt fpops: n_add=3 n_mul=4 n_div=2
; total fpops: n_add=22 n_mul=32 n_div=11, n_fpops=65
;           => 0.2444fpop/cycle

; this uses an internal sqrt, making it leaf & meaning no stack accesses are
; required (unlike the .unopt flavor which could run with any sqrtf impl.)

; NOTE: we store 1/l00, 1/l11, 1/l22 as l01, l12, l23; this makes trsm faster

  .code .text.potrfln4x4

; void potrfln4x4( float *L, int w );

  .lalign
  .type potrfln4x4, @function
potrfln4x4:
  ; input arguments
  .set  L i18
  .set  w i17
  ; locals we need to preserve across fn calls
  .set L0 v20
  .set l00 v20.0
  .set l01 v20.1
  .set L1 v21
  .set l10 v21.0
  .set l11 v21.1
  .set l12 v21.2
  .set L2 v22
  .set l20 v22.0
  .set l21 v22.1
  .set l22 v22.2
  .set l23 v22.3
  .set L3 v23
  .set l30 v23.0
  .set l31 v23.1
  .set l32 v23.2
  .set l33 v23.3

  lsu0.ldil      i30, .Lsqrtf || lsu1.ldih  i30, .Lsqrtf || cmu.cpii i11, i30
; l00 = A[0][0];
; l00 = sqrtf( l00 );
  lsu1.ld.32     i18, L       || bru.swp    i30          || iau.add   i0,  L w
    lsu1.ld.64.l  L1, i0      || lsu0.ldil   i8, 1.0f32  || iau.add   i0, i0 w
    lsu1.ldi.64.l L2, i0      || lsu0.ldih   i8, 1.0f32  || iau.add   i1, i0 w
    lsu1.ld.64.h  L2, i0      || lsu0.ldil   i7, 0xFFFF
    lsu1.ldi.64.l L3, i1      || lsu0.ldih   i7, 0x807F
    lsu1.ld.64.h  L3, i1      || lsu0.cp    i13, L
    cmu.cpzv    L0  1         || lsu0.cp    i12, w
  .unset L
  .unset w
  .set L i13
  .set w i12
  ; here: i18 = sqrtf(A[0][0])

; // L[0] all done!
; l10  = A[1][0];
; l20  = A[2][0];
; l30  = A[3][0];
; l10 /= l00;
; l20 /= l00;
; l30 /= l00;
; l11  = A[1][1];
; l21  = A[2][1];
; l22  = A[2][2];
; l31  = A[3][1];
; l32  = A[3][2];
; l33  = A[3][3];
  sau.div.f32  i0, i8 i18  || lsu0.cp          l00, i18     ; i0 = 1.0f / l00
  lsu0.ldil   i30, .Lsqrtf || lsu1.ldih        i30, .Lsqrtf
  lsu0.cp      i1, l10     || lsu1.cp           i2, l20
  lsu0.cp      i3, l30     || lsu1.cp           i4, l11
  iau.xor      i0, i0 i0   || vau.xor           v0, v0 v0
  lsu0.cp   v22.3, i0      ; cleanup L[2].3
  nop 6
  ; here, i0 is 1/L[0][0]
  sau.mul.f32     i1, i1 i0   || lsu0.cp       l01, i0
  sau.mul.f32     i2, i2 i0
  sau.mul.f32     i3, i3 i0   || bru.swp       i30
; L2 -= l20 * { 0 l10 l20 0 }
; L3 -= l30 * { 0 l10 l20 l30 }
; l11  = sqrtf( l11 - l10*l10 );
    sau.mul.f32   i5, i1 i1   || lsu0.cp      v0.1, i1
    lsu1.cp     v0.2, i2      || cmu.cpivr.x32  v1, i2
    vau.mul.f32   v1, v0 v1   || cmu.cpivr.x32  v2, i3
    sau.sub.f32  i18, i4 i5   || lsu0.cp      v0.3, i3      || lsu1.cp       l10, i1
    vau.mul.f32   v2, v0 v2   || lsu0.cp       l20, i2
    vau.sub.f32   L2, L2 v1   || lsu0.cp       l30, i3
  ; here: i18 = sqrtf(l11)

; // L[1]: all done
; l21 /= l11;
; l31 /= l11;
  sau.div.f32     i0, i8 i18  || lsu0.cp      l11, i18
  vau.sub.f32     L3, L3 v2   || lsu1.cp       i4, l22
  vau.xor         v0, v0 v0   || lsu0.cp       i1, l21
  lsu0.ldil      i30, .Lsqrtf || lsu1.ldih    i30, .Lsqrtf
  lsu0.cp         i2, l31
  nop 7
  ; here, i0 is 1/L[1][1]
  sau.mul.f32     i1, i1 i0   || lsu0.cp       l12, i0
  sau.mul.f32     i2, i2 i0
  bru.swp i30
; L3 -= l31 * { 0 0 l21 l31 }
; l22  = sqrtf( l22 - l21*l21 );
    sau.mul.f32   i3, i1 i1   || lsu0.cp     v0.2, i1
    cmu.cpivr.x32 v1, i2      || lsu0.cp     v0.3, i2
    vau.mul.f32   v1, v1 v0   || lsu0.cp      l21, i1
    sau.sub.f32  i18, i4 i3   || lsu0.cp      l31, i2
    nop
    vau.sub.f32   L3, L3 v1
  ; here: i18 = sqrtf(l22)

; // L[2]: all done
; l32 /= l22;
  sau.div.f32     i0, i8 i18  || lsu0.cp      l22, i18
  lsu0.ldil      i30, .Lsqrtf || lsu1.ldih    i30, .Lsqrtf
  lsu0.cp         i1, l32     || cmu.cpvi      i3, l33
  nop 9
  ; here, i0 is 1/L[2][2]
  sau.mul.f32     i1, i1 i0  || lsu0.cp       l23, i0
  nop
  bru.swp        i30
; l33 = sqrtf( l33 - l32*l32 );
    sau.mul.f32   i2, i1 i1   || lsu0.cp      l32, i1
    nop 2
    sau.sub.f32  i18, i3 i2
    nop 2
  ; here: i18 = sqrtf(l33)

  ; all done - return
  bru.jmp      i11
    lsu1.sti.64.l L0, L w     || iau.add       i0,   L w
    lsu1.sti.64.l L1, L w     || lsu0.sto.64.h L1,  i0 8
    lsu1.stxvi    L2, L w     || lsu0.cp      l33, i18
    nop
    lsu1.stxv     L3, L
    nop

;----------------------------------------------------------------------------

  .lalign
.Lsqrtf:
  .set   x i18
  .set   f i17
  .set   y i16
  .set exp i15
  .set nex i14

  iau.and       f, x  i7  || lsu0.ldil i1, 0          || lsu1.ldih i1, 0x3E80
  iau.or        f, f  i1  || lsu0.ldil i2, 1.18032f32 || lsu1.ldih i2, 1.18032f32
  sau.mul.f32   y, f i2
  iau.not      i0, i7     || lsu0.ldil i2, 0.41731f32
  iau.and     exp, x i0   || lsu0.ldih i2, 0.41731f32
  sau.add.f32   y, y i2
  lsu1.cp     nex, exp    || lsu0.ldil i2, .L.hlast
                             lsu0.ldih i2, .L.hlast
  sau.div.f32  i6, f y    || bru.rps   i2 2
   sau.mul.f32  y, y 0.5
   lsu0.ldil   i5, 0.707106781f32
   lsu0.ldih   i5, 0.707106781f32
   lsu0.ldih   i1, 0x0080
   iau.shl     i6, i1 8
   iau.not     i2, i6
   lsu0.ldil   i3, 0x0000
.L.hlast:
   lsu0.ldih   i3, 0x3f00
   lsu0.ldil   i4, 0x0000
   lsu0.ldih   i4, 0x7fc0
   iau.or      i4, x i4
   sau.add.f32  y, y i6
   iau.and     i6, exp i1
   nop
  iau.add     nex, exp i1 || sau.mul.f32 y, y i5 || peu.pc1i neq
  iau.sub     nex, nex i3
  iau.shr.i32 nex, nex 1  || bru.jmp      i30
    iau.and   nex, nex i0 || cmu.cmii.i32 exp  i0
    iau.add     y, y nex  || sau.or       exp, exp exp
    lsu0.cp     y, x      || peu.pc1c eq
    lsu0.ldil   y, 0      || peu.pc1s eq
    sau.and   i18, y i2   || iau.add       i0, x 0
    lsu0.cp   i18, i4     || peu.pc1i lt
  .size potrfln4x4, . - potrfln4x4

  .end
