.version 00.50.02
.set PI_div180         0x3c8efa35

;void binary_test_asm(uchar *center, int step, float sin_, float cos_, float* pattern, uchar *descriptor, int dsize);
; i18: center
; i17: step
; i16: sin_
; i15: cos_
; i14: pattern
; i13: descriptor
; i12: dsize
.set center  i18
.set step    i17
.set sin_    i16
.set cos_    i15
.set pattern i14
.set descriptor i13
.set dsize i12
.code .text.binary_test_asm
.lalign
  .type binary_test_asm, @function
binary_test_asm:
; i11..i0: temp
; v20: descriptor
; v19: t1
; v18: t0
; v17: sin(angle*PI/180)
; v16: cos(angle*PI/180)
; v15..v0: temp
LSU0.LDI.64.l v0,  pattern
LSU0.LDI.64.h v0,  pattern
LSU0.LDI.64.l v1,  pattern
LSU0.LDI.64.h v1,  pattern
LSU0.LDI.64.l v2,  pattern
LSU0.LDI.64.h v2,  pattern
LSU0.LDI.64.l v3,  pattern
LSU0.LDI.64.h v3,  pattern
LSU0.LDI.64.l v4,  pattern
LSU0.LDI.64.h v4,  pattern
LSU0.LDI.64.l v5,  pattern
LSU0.LDI.64.h v5,  pattern    || CMU.CPII.f16l.f32 i1, i1
LSU0.LDI.64.l v6,  pattern    || CMU.CPII.f16l.f32 i2, i1
LSU0.LDI.64.h v6,  pattern    || CMU.CPIVR.x32 v16, i15
LSU0.LDI.64.l v7,  pattern    || CMU.CPIVR.x32 v17, i16
LSU0.LDI.64.h v7,  pattern    || CMU.VDILV.x32 v0,  v1,  v0,  v1
VAU.MUL.f32 v8,  v0,  v16 
VAU.MUL.f32 v9,  v1,  v17
VAU.MUL.f32 v0,  v0,  v17
VAU.MUL.f32 v1,  v1,  v16 || CMU.VDILV.x32 v2,  v3,  v2,  v3
VAU.MUL.f32 v10, v2,  v16
VAU.MUL.f32 v11, v3,  v17
VAU.MUL.f32 v2,  v2,  v17
VAU.MUL.f32 v3,  v3,  v16 || CMU.VDILV.x32 v4,  v5,  v4,  v5
VAU.MUL.f32 v12, v4,  v16
VAU.MUL.f32 v13, v5,  v17
VAU.MUL.f32 v4,  v4,  v17
VAU.MUL.f32 v5,  v5,  v16 || CMU.VDILV.x32 v6,  v7,  v6,  v7
VAU.MUL.f32 v14, v6,  v16
VAU.MUL.f32 v15, v7,  v17
VAU.MUL.f32 v6,  v6,  v17
VAU.MUL.f32 v7,  v7,  v16 || IAU.SUBSU dsize, dsize, 0x01

.lalign
binary_test_loop:
VAU.ADD.f32 v8,  v9,  v8 
VAU.SUB.f32 v9,  v1,  v0
VAU.ADD.f32 v10, v11, v10     || CMU.CPZV           v20, 0
VAU.SUB.f32 v11, v3,  v2      || LSU1.LDIL          i1, 0xFFFF
VAU.ADD.f32 v12, v13, v12     || LSU1.LDIH          i1, 0xFFFF
VAU.SUB.f32 v13, v5,  v4      || CMU.CPIVR.x32      v21, i1
VAU.ADD.f32 v14, v15, v14     || CMU.CPIVR.x32      v18, i17
VAU.SUB.f32 v15, v7,  v6      || CMU.CMZ.f32        v8 
VAU.ADD.f32 v8,  v8,  0.5                                         || LSU0.LDI.64.l v0,  pattern           || PEU.PVV32 0x3
VAU.SUB.f32 v8,  v8,  0.5     || CMU.CMZ.f32        v9            || LSU0.LDI.64.h v0,  pattern           || PEU.PVV32 0x4
VAU.ADD.f32 v9,  v9,  0.5                                         || LSU0.LDI.64.l v1,  pattern           || PEU.PVV32 0x3
VAU.SUB.f32 v9,  v9,  0.5     || CMU.CMZ.f32        v10           || LSU0.LDI.64.h v1,  pattern           || PEU.PVV32 0x4
VAU.ADD.f32 v10, v10, 0.5     || CMU.CPVV.f32.i32s  v8,  v8       || LSU0.LDI.64.l v2,  pattern           || PEU.PVV32 0x3
VAU.SUB.f32 v10, v10, 0.5     || CMU.CMZ.f32        v11           || LSU0.LDI.64.h v2,  pattern           || PEU.PVV32 0x4
VAU.ADD.f32 v11, v11, 0.5     || CMU.CPVV.f32.i32s  v9,  v9       || LSU0.LDI.64.l v3,  pattern           || PEU.PVV32 0x3
VAU.SUB.f32 v11, v11, 0.5     || CMU.CMZ.f32        v12           || LSU0.LDI.64.h v3,  pattern           || PEU.PVV32 0x4
VAU.ADD.f32 v12, v12, 0.5     || CMU.CPVV.f32.i32s  v10, v10      || LSU0.LDI.64.l v4,  pattern           || PEU.PVV32 0x3
VAU.SUB.f32 v12, v12, 0.5     || CMU.CMZ.f32        v13           || LSU0.LDI.64.h v4,  pattern           || PEU.PVV32 0x4
VAU.ADD.f32 v13, v13, 0.5     || CMU.CPVV.f32.i32s  v11, v11      || LSU0.LDI.64.l v5,  pattern           || PEU.PVV32 0x3
VAU.SUB.f32 v13, v13, 0.5     || CMU.CMZ.f32        v14           || LSU0.LDI.64.h v5,  pattern           || PEU.PVV32 0x4
VAU.ADD.f32 v14, v14, 0.5     || CMU.CPVV.f32.i32s  v12, v12      || LSU0.LDI.64.l v6,  pattern           || PEU.PVV32 0x3
VAU.SUB.f32 v14, v14, 0.5     || CMU.CMZ.f32        v15           || LSU0.LDI.64.h v6,  pattern           || PEU.PVV32 0x4
VAU.ADD.f32 v15, v15, 0.5     || CMU.CPVV.f32.i32s  v13, v13      || LSU0.LDI.64.l v7,  pattern           || PEU.PVV32 0x3
VAU.SUB.f32 v15, v15, 0.5                                         || LSU0.LDI.64.h v7,  pattern           || PEU.PVV32 0x4
VAU.MUL.i32 v8,  v8,  v18     || CMU.CPVV.f32.i32s  v14, v14
VAU.MUL.i32 v10, v10, v18
VAU.MUL.i32 v12, v12, v18     || CMU.CPVV.f32.i32s  v15, v15
VAU.MUL.i32 v14, v14, v18     || CMU.CMZ.i32   dsize
                                 CMU.VDILV.x32 v0,  v1,  v0,  v1
VAU.ADD.i32 v9,  v9,  v8      || CMU.VDILV.x32 v2,  v3,  v2,  v3
VAU.ADD.i32 v11, v11, v10     || CMU.VDILV.x32 v4,  v5,  v4,  v5
VAU.ADD.i32 v13, v13, v12     || CMU.VDILV.x32 v6,  v7,  v6,  v7
VAU.ADD.i32 v15, v15, v14     || CMU.CPVID     i0,  v9.0
VAU.MUL.f32 v8,  v0,  v16     || CMU.CPVID     i0,  v9.2          || LSU0.LDX.32.u8.u32 i4,  i18, i0  || LSU1.LDX.32.u8.u32 i5,  i18, i1
VAU.MUL.f32 v9,  v1,  v17     || CMU.CPVID     i0,  v11.0         || LSU0.LDX.32.u8.u32 i2,  i18, i0  || LSU1.LDX.32.u8.u32 i3,  i18, i1
VAU.MUL.f32 v0,  v0,  v17     || CMU.CPVID     i0,  v11.2         || LSU0.LDX.32.u8.u32 i6,  i18, i0  || LSU1.LDX.32.u8.u32 i7,  i18, i1
VAU.MUL.f32 v1,  v1,  v16     || CMU.CPVID     i0,  v13.0         || LSU0.LDX.32.u8.u32 i2,  i18, i0  || LSU1.LDX.32.u8.u32 i3,  i18, i1
VAU.MUL.f32 v10, v2,  v16     || CMU.CPVID     i0,  v13.2         || LSU0.LDX.32.u8.u32 i8,  i18, i0  || LSU1.LDX.32.u8.u32 i9,  i18, i1
VAU.MUL.f32 v11, v3,  v17     || CMU.CPVID     i0,  v15.0         || LSU0.LDX.32.u8.u32 i2,  i18, i0  || LSU1.LDX.32.u8.u32 i3,  i18, i1
VAU.MUL.f32 v2,  v2,  v17     || CMU.CPVID     i0,  v15.2         || LSU0.LDX.32.u8.u32 i10, i18, i0  || LSU1.LDX.32.u8.u32 i11, i18, i1
VAU.MUL.f32 v3,  v3,  v16                                         || LSU0.LDX.32.u8.u32 i2,  i18, i0  || LSU1.LDX.32.u8.u32 i3,  i18, i1
VAU.MUL.f32 v12, v4,  v16     || CMU.VSZM.BYTE i5,  i3,  [D0DD]   || IAU.FINS i4,  i2,  0x10, 0x08
VAU.MUL.f32 v13, v5,  v17     || CMU.CPIV      v19.0, i5          || LSU1.CP  v18.0,  i4 
VAU.MUL.f32 v4,  v4,  v17     || CMU.VSZM.BYTE i7,  i3,  [D0DD]   || IAU.FINS i6,  i2,  0x10, 0x08
VAU.MUL.f32 v5,  v5,  v16     || CMU.CPIV      v19.1, i7          || LSU1.CP  v18.1,  i6 
VAU.MUL.f32 v14, v6,  v16     || CMU.VSZM.BYTE i9,  i3,  [D0DD]   || IAU.FINS i8,  i2,  0x10, 0x08
VAU.MUL.f32 v15, v7,  v17     || CMU.CPIV      v19.2, i9          || LSU1.CP  v18.2,  i8
VAU.MUL.f32 v6,  v6,  v17     || CMU.VSZM.BYTE i11, i3,  [D0DD]   || IAU.FINS i10, i2,  0x10, 0x08     || BRU.BRA binary_test_loop    || PEU.PCCX.NEQ 0
VAU.MUL.f32 v7,  v7,  v16     || CMU.CPIV      v19.3, i11         || LSU1.CP  v18.3,  i10              || BRU.JMP i30                 || PEU.PCCX.EQ  0
                                 CMU.CMVV.u16  v18, v19
								 nop
VAU.CMB.WORD v20, v21, [EEEE]                                     || PEU.PVV16 0x4
IAU.SUBSU dsize, dsize, 0x01      || CMU.VNZ.x16 i0,  v20,  0
LSU0.STI.8  i0,  i13
nop 
