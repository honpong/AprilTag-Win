  .version 00.50.01

  .code .text
; C = ( C - sum[k=0:m:4]{ A@(0,k) * (B@(0,k))' } ) / D'
; E -= C * C'

; void potrfln4x4upd( float *C, float *E,
;                     const float *A, const float *B, int m,
;                     const float *D,
;                     int wCEA, int wBD );
potrfln4x4upd:
  ; inputs
  .set    C i18
  .set    E i17
  .set    A i16
  .set    B i15
  .set    m i14
  .set    D i13
  .set wCEA i12
  .set  wBD i11
  ; local ints
  .set  a30 i7
  .set  a31 i8
  .set  a32 i9
  .set  a33 i10
  .set  a10 i5
  .set  n00 i6
  ; i0:i1 : temps (mostly for hsumming An*Bm)
  ; local vectors
  .set  A0 v20
  .set  A1 v21
  .set  A2 v22
  .set  a3 v23
  .set  tB0 v16 ; transposed B
  .set  tB1 v17
  .set  tB2 v18
  .set  tB3 v19
  .set  B0 v12
  .set  B1 v13
  .set  B2 v14
  .set  B3 v15
  .set  C0 v8
  .set  C1 v9
  .set  C2 v10
  .set  C3 v11
  ; v7 : temp for doing A*B SIMD mul
  ; v0:v2   : temps for collecting C0/C1/C2 elements

  iau.shr.u32     m,  m 2    || lsu0.cp    i0, C
  bru.bra      .Ltrsm        || peu.pc1i  lte
    lsu1.ldxvi   C0, i0 wCEA || lsu0.cp    i2, A
    lsu0.ldi.64 a10,  D wBD     ; this really loads n00; a10 will be reloaded shortly
    lsu1.ldxvi   C1, i0 wCEA || lsu0.cp    i3, B
    lsu0.ld.32  a10,  D
    lsu1.ldxv    C2, i0      || iau.add    i0, i0 wCEA
    lsu0.ldxv    C3, i0
  lsu1.ldxvi     A0, i2 wCEA
  lsu0.ldxvi     B0, i3 wBD
  lsu1.ldxvi     A1, i2 wCEA
  lsu0.ldxvi     B1, i3 wBD  || iau.xor    i0, i0 i0
  lsu1.ldxvi     A2, i2 wCEA || iau.incs    A, 16
  lsu0.ldxvi     B2, i3 wBD  || iau.incs    B, 16
  lsu1.ldi.64   a30, i2      || iau.incs    m, -1
  lsu0.ldxv      B3, i3      || sau.or     i3, B B
  lsu1.ld.64    a32, i2      || cmu.cpii   i2, A
  bru.bra    __mul_loop_last || peu.pc1i   eq
    vau.xor      v7, v7 v7   || lsu0.ldil  i1, __mul_loop_end
    vau.accpz.f32    C3      || lsu0.ldih  i1, __mul_loop_end
    nop 3
    cmu.tp4r    tB0, B0
  vau.mul.f32    v7, A0  B0  || bru.rpl      i1  m                           || lsu1.ldxvi   A0, i2 wCEA
  vau.mul.f32    v7, A0  B1  || sau.sumx.f32 i0, v7 || lsu0.cp     v2.0, i0
  vau.mul.f32    v7, A0  B2                         || lsu0.cp     v2.1, i0  || lsu1.ldxvi  tB0, i3 wBD
  vau.mul.f32    v7, A0  B3  || sau.sumx.f32 i0, v7 || cmu.cpivr.x32 a3, a30
  vau.macn.f32       a3 tB0  || sau.sumx.f32 i0, v7 || lsu0.cp     v2.2, i0  || lsu1.ldxvi   A1, i2 wCEA
  vau.mul.f32    v7, A1  B0  || sau.sumx.f32 i1, v7 || cmu.cpivr.x32 a3, a31 || iau.incs      A, 16
  vau.mul.f32    v7, A1  B1  || sau.sumx.f32 i0, v7                          || lsu1.ldxvi   B1, i3 wBD
  vau.macn.f32       a3 tB1                         || lsu0.cp     v2.3, i0  || iau.incs      B, 16
  vau.mul.f32    v7, A1  B2  || sau.sumx.f32 i1, v7 || lsu0.cp     v0.0, i0  || lsu1.ldxvi   B2, i3 wBD
  vau.mul.f32    v7, A1  B3  || sau.sumx.f32 i0, v7 || cmu.cpivr.x32 a3, a32
  vau.macn.f32       a3 tB2                         || lsu0.cp     v0.1, i0  || lsu1.ldxvi   B3, i3 wBD
  vau.mul.f32    v7, A2  B0  || sau.sumx.f32 i0, v7 || lsu0.cp     v0.2, i1
__mul_loop_end:
    vau.mul.f32  v7, A2  B1  || sau.sumx.f32 i0, v7 || cmu.cpivr.x32 a3, a33 || iau.or       i3,  B B
    vau.macn.f32    a3 tB3                          || lsu0.cp     v0.3, i0  || lsu1.ldxvi   A2, i2 wCEA
    vau.mul.f32  v7, A2  B2  || sau.sumx.f32 i0, v7 || lsu0.cp     v1.0, i1  || cmu.cpvv     B0, tB0
    vau.mul.f32  v7, A2  B3                         || lsu0.cp     v1.1, i0  || lsu1.ldi.64 a30, i2
    vau.sub.f32  C2, C2 v2   || sau.sumx.f32 i0, v7 || lsu0.cp     v1.2, i0  || lsu1.ld.64  a32, i2
    vau.sub.f32  C0, C0 v0   || sau.sumx.f32 i0, v7 || lsu0.cp     v1.3, i0  || iau.or       i2,  A A
    vau.sub.f32  C1, C1 v1                          || cmu.tp4r     tB0, B0
__mul_loop_last:
  ; last iteration
  vau.mul.f32    v7, A0  B0
  vau.mul.f32    v7, A0  B1  || sau.sumx.f32 i0, v7 || lsu0.cp     v2.0, i0
  vau.mul.f32    v7, A0  B2                         || lsu0.cp     v2.1, i0
  vau.mul.f32    v7, A0  B3  || sau.sumx.f32 i0, v7 || cmu.cpivr.x32 a3, a30
  vau.macn.f32       a3 tB0  || sau.sumx.f32 i0, v7 || lsu0.cp     v2.2, i0
  vau.mul.f32    v7, A1  B0  || sau.sumx.f32 i0, v7 || cmu.cpivr.x32 a3, a31
  vau.mul.f32    v7, A1  B1  || sau.sumx.f32 i0, v7 || lsu0.cp     v2.3, i0
  vau.macn.f32       a3 tB1                         || cmu.cpivr.x32 a3, a32
  vau.mul.f32    v7, A1  B2  || sau.sumx.f32 i0, v7 || lsu0.cp     v0.0, i0
  vau.mul.f32    v7, A1  B3  || sau.sumx.f32 i0, v7 || lsu0.cp     v0.1, i0
  vau.macn.f32       a3 tB2                         || lsu0.cp     v0.2, i0
  vau.mul.f32    v7, A2  B0  || sau.sumx.f32 i0, v7 || lsu0.cp     v0.3, i0
  vau.mul.f32    v7, A2  B1  || sau.sumx.f32 i0, v7
  vau.mul.f32    v7, A2  B2                         || lsu0.cp     v1.0, i0
  vau.mul.f32    v7, A2  B3  || sau.sumx.f32 i0, v7 || lsu0.cp     v1.1, i0
  vau.sub.f32    C2, C2 v2   || sau.sumx.f32 i0, v7 || cmu.cpivr.x32 a3, a33
  vau.sub.f32    C0, C0 v0   || sau.sumx.f32 i0, v7 || lsu0.cp     v1.2, i0
  vau.macnw.f32  C3, a3 tB3  || sau.sumx.f32 i0, v7 || lsu0.cp     v1.3, i0
  vau.sub.f32    C1, C1 v1
                                                      lsu0.cp     v2.0, i0
                                                      lsu0.cp     v2.1, i0
                                                      lsu0.cp     v2.2, i0
  ; note the awkwardly placed label; this is such that
  ; a) ldxv C2's 1st writeback cycle is the one AFTER the sub writeback (when
  ;    skipping the gemm step via m==0)
  ; b) ldxv C3's 2nd writeback cycle is the one in which the ld.64 a32 is
  ;    issued (when skipping gemm)
  ; c) C2 subtraction writeback will happen long before the transpose, in the
  ;    normal case (when gemm is performed via m>0)
.Ltrsm:
                               iau.add   i1, D 8   || lsu0.cp     v2.3, i0
  vau.sub.f32    C2, C2 v2  || iau.add    D, D wBD
  ; remap some no-longer-needed registers
  .unset A
  .unset B
  .unset m
  .set   n11 i14
  .set   a20 i15
  .set   a21 i16
  .set   n22 i4
  lsu0.ld.32    n11, i1     || iau.xor        i1, i1 i1
  lsu1.ldi.64   a20,  D wBD
  lsu0.ldo.32   n22,  D 12  || lsu1.ldih      i1, 1.0f32
  lsu1.ldi.64   a30,  D
  lsu1.ld.64    a32,  D     || cmu.cpivr.x32  A0, n00
  cmu.tp4r       C0, C0
  vau.mul.f32    C0, C0 A0  || cmu.cpivr.x32  B0, a10
  nop 2
  vau.mul.f32    B0, C0 B0  || cmu.cpivr.x32  B1, a20
  vau.mul.f32    B1, C0 B1  || cmu.cpivr.x32  B2, a30
  vau.mul.f32    B2, C0 B2  || sau.div.f32   a33,  i1 a33
  vau.sub.f32    C1, C1 B0  || lsu0.cp        i0,  E
  vau.sub.f32    C2, C2 B1  || lsu1.ldi.32   n00,  i0 wCEA
  vau.sub.f32    C3, C3 B2  || cmu.cpivr.x32  A1, n11
  vau.mul.f32    C1, C1 A1  || cmu.cpivr.x32  B0, a21
  nop 2
  vau.mul.f32    B0, C1 B0  || cmu.cpivr.x32  B1, a31
  vau.mul.f32    B1, C1 B1  || lsu1.ldi.64   a20,  i0 wCEA
  nop
  vau.sub.f32    C2, C2 B0  || lsu1.ldxvi     B2,  i0 wCEA
  vau.sub.f32    C3, C3 B1  || cmu.cpivr.x32  A2, n22
  nop
  vau.mul.f32    C2, C2 A2  || cmu.cpivr.x32  B0, a32
  nop 2
  vau.mul.f32    B0, C2 B0  || lsu0.cp        i1, i0
  nop 2
  vau.sub.f32    C3, C3 B0  || cmu.cpivr.x32  a3, a33
  nop 2
  vau.mul.f32    C3, C3 a3  || lsu1.ldxv      B3, i0
  nop
  nop
  cmu.tp4r       C0, C0
  vau.mul.f32    v7, C3 C0  || lsu1.stxvi     C0, C wCEA
  vau.mul.f32    v7, C3 C1
  vau.mul.f32    v7, C3 C2  || lsu1.stxvi     C1, C wCEA
  vau.mul.f32    v7, C3 C3  || sau.sumx.f32   i0, v7
  vau.mul.f32    v7, C2 C0  || sau.sumx.f32   i0, v7     || lsu1.stxvi C2, C wCEA
  vau.mul.f32    v7, C2 C1  || sau.sumx.f32   i0, v7
  vau.mul.f32    v7, C2 C2  || sau.sumx.f32   i0, v7     || lsu1.stxv  C3, C
  vau.mul.f32    v7, C0 C0  || sau.sumx.f32   i0, v7
  vau.mul.f32    v7, C1 C0  || sau.sumx.f32   i0, v7     || lsu0.cp v23.0, i0 ; v23:=a3
  vau.mul.f32    v7, C1 C1  || sau.sumx.f32   i0, v7     || lsu0.cp v23.1, i0
                               sau.sumx.f32   i0, v7     || lsu0.cp v23.2, i0
                               sau.sumx.f32   i0, v7     || lsu0.cp v23.3, i0
                               sau.sumx.f32   i0, v7     || lsu0.cp v22.0, i0
                                                            lsu0.cp v22.1, i0
  vau.sub.f32    B3, B3 a3                               || lsu0.cp v22.2, i0
                               sau.sub.f32   n00, n00 i0        ; e00 in n00
  bru.jmp       i30         || sau.sub.f32   a20, a20 i0        ; e10, e11 in a20, a21
    lsu1.stxv    B3, i1     || sau.sub.f32   a21, a21 i0
                               vau.sub.f32    B2,  B2 A2
    lsu1.sti.32 n00, E wCEA
    lsu1.sti.64 a20, E wCEA
    lsu1.stxv    B2, E
    nop
