
.version 00.50.02
;void ic_angle(float x,float y, int step, u8 image[], int vUmax[], int orb_half_patch_size, float* sin_, float* cos_)

.code .text.ic_angle_asm
.lalign
ic_angle_asm:
  .set x      i18
  .set y      i17
  .set step   i16
  .set image  i15
  .set vUmax  i14
  .set orb    i13
  .set sin_   i12
  .set cos_   i11
  .set center i3
  .set u      i9
  .set m_01   i0
  .set m_10   i1
  lsu0.ldih i10 0x3F00  || lsu1.ldil i10 0      || cmu.cmz.f32 x         || iau.add i6 orb 0 || sau.xor i9 i9 i9
  peu.pc1c GTE          || sau.add.f32 x x i10  || cmu.cmz.f32 y
  peu.pc1c LT           || sau.sub.f32 x x i10  || cmu.cmz.f32 y
  peu.pc1c GTE          || sau.add.f32 y y i10
  peu.pc1c LT           || sau.sub.f32 y y i10
  cmu.cpii.f32.i32s x x || lsu0.ldil i5 0x0000  || lsu1.ldih i5 0x0001   || iau.shl i7 i6 16
                           iau.or i6 i6 i7      || lsu1.ldil m_01 0x0000
  cmu.cpii.f32.i32s y y || sau.sub.i16 i5 i5 i6 || iau.sub u m_01 orb
  nop
  iau.add x x image     || sau.add.i16 i8 i5 2  || lsu1.ldil m_10 0x0000
  iau.mul y y step      || lsu0.cp v1.0 i5      || sau.add.i16 i8 i5 4  || lsu1.ldil i10 0x0008
                           lsu0.cp v1.1 i8      || sau.add.i16 i8 i5 6  || lsu1.ldih i10 0x0000
  iau.add center y x    || lsu0.cp v1.2 i8      || cmu.cpivr.x16 v2 i10
  iau.sub i4 center orb || lsu0.cp v1.3 i8      || cmu.cpivr.x16 v3 orb || vau.xor v5 v5 v5
  lsu0.ld.128.u8.u16 v0 i4
  nop 2

  loop31:
    cmu.cmvv.i16 v1 v3 || iau.incs u 8 || sau.add.i32 i4 i4 8
    peu.pvv16 GT       || vau.sub.i16 v1 v1 v1
    cmu.cmii.i32 u orb
    peu.pc1c LT  || bru.bra loop31 || lsu0.ld.128.u8.u16 v0 i4
    vau.mul.i16 v4 v1 v0
    nop
    vau.add.i16 v1 v1 v2
    vau.add.i16 v5 v5 v4
    nop 2

  .set v i5
  .set v_sum i4
  .set d i2

  iau.incs vUmax 4   || lsu0.ldil v 1 || sau.sumx.i16 m_10 v5
  lsu0.ld.32 d vUmax
  cmu.cpii.i16.i32 m_10 m_10
  nop 2

  loop22:
    iau.xor v_sum v_sum v_sum
    iau.xor i10 i10 i10
    iau.xor u u u
    iau.sub u u d
    iau.sub i15 center d    || cmu.cpii.i32.i16s i10 u
    iau.mul i6 v step
    lsu0.ldih i17 0x0001    || lsu1.ldil i17 0x0000
    lsu0.cp i7 i6           || iau.shl i18 i10 16
    iau.add i6 i6 i15       || sau.add.i32 i18 i18 i17
    iau.sub i7 i15 i7       || lsu1.ld.128.u8.u16 v1 i6
    iau.or  i10 i10 i18     || lsu1.ld.128.u8.u16 v2 i7
    lsu0.cp v0.0 i10 || sau.add.i16 i18 i10 2 || lsu1.ldil i17 0x0008
                        sau.add.i16 i18 i10 4 || cmu.cpivr.x16 v9 d
    lsu0.cp v0.1 i18 || sau.add.i16 i18 i10 6 || cmu.cpivr.x16 v11 i17
    lsu0.cp v0.2 i18
    lsu0.cp v0.3 i18 || cmu.cpzv v6 1
    cmu.cpzv v4 1

    loop221:

      iau.incs u  8 || sau.add.i32 i7 i7 8 || cmu.cmvv.i16 v0 v9
      vau.add.i16 v4 v1 v2            || peu.pvv16 LTE
      vau.sub.i16 v5 v1 v2            || peu.pvv16 LTE
      vau.mul.i16 v4 v0 v4            || cmu.cmii.i32 u d || iau.incs i6 8
      peu.pc1c LTE || bru.bra loop221 || lsu0.ld.128.u8.u16 v1 i6
      peu.pc1c LTE                    || lsu0.ld.128.u8.u16 v2 i7
      vau.add.i16 v0 v0 v11
      vau.add.i16 v6 v6 v5
      vau.add.i16 v7 v7 v4
      cmu.cpzv v4 1
      nop

    iau.incs vUmax 4 || cmu.cpvv.i16.i32 v12 v7
    cmu.alignvec v7 v7 v7 8
    cmu.cpvv.i16.i32 v12 v7 || sau.sumx.i32 i18 v12
    cmu.cmii.i32 v orb      || sau.sumx.i32 i17 v12
    peu.pc1c LT || bru.bra loop22 || lsu0.ld.32 d vUmax
    iau.add i18 i18 i17           || sau.sumx.i16 v_sum v6
    nop
    cmu.cpii.i16.i32 v_sum v_sum
    iau.mul i8 v v_sum
    sau.add.i32 v v 1
    iau.add  m_01 m_01 i8    || sau.add.i32 m_10 m_10 i18

    cmu.cpii.i32.f32 m_01 m_01 || lsu0.ldil i7  0x0000  || lsu1.ldih i7 0x3F80
    cmu.cpii.i32.f32 m_10 m_10
    sau.mul.f32 i2 m_01 m_01  || lsu0.cp i9 m_01
    sau.mul.f32 i3 m_10 m_10  || lsu0.cp i10 m_10
    nop 2
    sau.add.f32 i18 i2 i3 //d1 = ( fm_01 * fm_01 + fm_10 * fm_10);
    iau.xor i8 i8 i8

//============Start Sqrtf==========
  .unsetall
  .set   x i18
  .set   f i17
  .set   y i16
  .set exp i15
  .set nex i14 // new exponent

  // {f, exp+126} = frexp_v4f(x)//
  // f *= 0.5
  // y = f * 1.18032 + 0.41731
  // optimizations: assume positive, don't check for 0/Inf/NaN input (handled at the end)
                             lsu0.ldil i0, 0xFFFF     || lsu1.ldih i0, 0x807F
  iau.and       f, x  i0  || lsu0.ldil i1, 0          || lsu1.ldih i1, 0x3E80 // i1=0x3e800000
  iau.or        f, f  i1  || lsu0.ldil i2, 1.18032f32 || lsu1.ldih i2, 1.18032f32
  sau.mul.f32   y, f i2                               // y=f*1.18032
  iau.not      i0, i0     || lsu0.ldil i2, 0.41731f32 // i0=0x7f800000
  iau.and     exp, x i0   || lsu0.ldih i2, 0.41731f32
  sau.add.f32   y, y i2                               // y=f*1.18032+0.41731 i1=0x00800000
  lsu1.cp     nex, exp    || lsu0.ldil i2, .Lhlast
                             lsu0.ldih i2, .Lhlast
  sau.div.f32  i6, f y    || bru.rps   i2 2 // i6=f/y
  sau.mul.f32   y, y 0.5       // y*=0.5
  lsu0.ldil    i5, 0.707106781f32
  lsu0.ldih    i5, 0.707106781f32
  lsu0.ldih    i1, 0x0080      // i1=0x00800000
  iau.shl      i6, i1 8        // i6=0x80000000
  iau.not      i2, i6          // i2=0x7fffffff
  lsu0.ldil    i3, 0x0000
.Lhlast:
    lsu0.ldih  i3, 0x3f00      // i3=0x3f000000
    lsu0.ldil  i4, 0x0000
    lsu0.ldih  i4, 0x7fc0      // i4=qNaN
    iau.or     i4, x i4
    sau.add.f32 y, y i6 // y=y*0.5+f/y
    nop
    iau.and    i6, exp i1
  iau.add     nex, exp i1 || sau.mul.f32 y, y i5 || peu.pc1i neq // if(exp&1)nex=exp+1,y*=sqrt(.5)
  iau.sub     nex, nex i3
  iau.shr.i32 nex, nex 1
    iau.and   nex, nex i0 || cmu.cmii.i32 exp  i0
    iau.add     y, y nex  || sau.or       exp, exp exp
  // if( exp==0xff ) y=x//
    lsu0.cp     y, x      || peu.pc1c eq
  // if( !exp ) y=0.0//
    lsu0.ldil   y, 0      || peu.pc1s eq
    sau.and   i18, y i2   || iau.add       i0, x 0
  // if( x<0 ) y=qNaN//
    lsu0.cp   i18, i4     || peu.pc1i lt
//====================End Sqrtf=========
    cmu.cmz.i32 i18
    peu.pc1c NEQ    || sau.div.f32 i7 i10 i18
    peu.pc1c NEQ    || sau.div.f32 i8 i9 i18
    nop 5
    bru.jmp i30
    nop 4
    lsu0.st.32 i7 i11
    lsu0.st.32 i8 i12
.end
