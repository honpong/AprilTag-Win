  .version 00.50.01

; 48 instructions, 48instr/value

; float sqrtf( float x )
; {
;   int exp;
;   float f = frexp( x, &exp );
;   f *= 0.5;
;   float y = 0.41731 + 1.18032*f;
;   y = y * 0.5 + f / y;
;   y = y * 0.5 + f / y;
;   if( exp & 1 ) {
;     y *= __SQRT_HALF;
;     ++exp;
;   }
;   exp >>= 1;
;   y = ldexpf( y, exp );
;   if( isinf( x )|| isnan( x ))
;     y = x;
;   if( !x )
;     y = 0;
;   if( x < 0 )
;     y = NAN;
;   return y;
; }


  .code .text.sqrtf
  .type sqrtf, @function
sqrtf:
  .set   x i18
  .set   f i17
  .set   y i16
  .set exp i15
  .set nex i14 ; new exponent

  ; {f, exp+126} = frexp_v4f(x);
  ; f *= 0.5
  ; y = f * 1.18032 + 0.41731
  ; optimizations: assume positive, don't check for 0/Inf/NaN input (handled at the end)
                             lsu0.ldil i0, 0xFFFF     || lsu1.ldih i0, 0x807F
  iau.and       f, x  i0  || lsu0.ldil i1, 0          || lsu1.ldih i1, 0x3E80 ; i1=0x3e800000
  iau.or        f, f  i1  || lsu0.ldil i2, 1.18032f32 || lsu1.ldih i2, 1.18032f32
  sau.mul.f32   y, f i2                               ; y=f*1.18032
  iau.not      i0, i0     || lsu0.ldil i2, 0.41731f32 ; i0=0x7f800000
  iau.and     exp, x i0   || lsu0.ldih i2, 0.41731f32
  sau.add.f32   y, y i2                               ; y=f*1.18032+0.41731 i1=0x00800000
  lsu1.cp     nex, exp    || lsu0.ldil i2, .Lhlast
                             lsu0.ldih i2, .Lhlast
  sau.div.f32  i6, f y    || bru.rps   i2 2 ; i6=f/y
  sau.mul.f32   y, y 0.5       ; y*=0.5
  lsu0.ldil    i5, 0.707106781f32
  lsu0.ldih    i5, 0.707106781f32
  lsu0.ldih    i1, 0x0080      ; i1=0x00800000
  iau.shl      i6, i1 8        ; i6=0x80000000
  iau.not      i2, i6          ; i2=0x7fffffff
  lsu0.ldil    i3, 0x0000
.Lhlast:
    lsu0.ldih  i3, 0x3f00      ; i3=0x3f000000
    lsu0.ldil  i4, 0x0000
    lsu0.ldih  i4, 0x7fc0      ; i4=qNaN
    iau.or     i4, x i4
    sau.add.f32 y, y i6 ; y=y*0.5+f/y
    nop
    iau.and    i6, exp i1
  iau.add     nex, exp i1 || sau.mul.f32 y, y i5 || peu.pc1i neq ; if(exp&1)nex=exp+1,y*=sqrt(.5)
  iau.sub     nex, nex i3
  iau.shr.i32 nex, nex 1  || bru.jmp      i30
    iau.and   nex, nex i0 || cmu.cmii.i32 exp  i0
    iau.add     y, y nex  || sau.or       exp, exp exp
  ; if( exp==0xff ) y=x;
    lsu0.cp     y, x      || peu.pc1c eq
  ; if( !exp ) y=0.0;
    lsu0.ldil   y, 0      || peu.pc1s eq
    sau.and   i18, y i2   || iau.add       i0, x 0
  ; if( x<0 ) y=qNaN;
    lsu0.cp   i18, i4     || peu.pc1i lt
  .size sqrtf, . - sqrtf

  .unsetall

  .end
