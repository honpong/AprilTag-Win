///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///
// FAST9 corner detection, early exclusion and pixel score calculation included
// input  : i18, pointer to parameters
// output : none


.version 00.51.04

.data .data.fast9M2_asm
.align 4
	possitionsOffsets:
        .int 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
    fast9M2_posValid:
		.fill 1920, 4, 0
	fast9M2_conv:
		.fill 1920, 1, 0
.code .text.fast9M2_asm      

//                   i18,         i17,         i16,       i15,             i14     
//void fast9M2(UInt8** row, UInt8* score, UInt32 *base, UInt32 thresh, UInt32 width)
//-8    -4         i19,
//addTostackValue,nrOfPoints,lineForPosiiton width*4+4
//
//                       i18            i17             i16             i15              i14
//void fastExclude(UInt8** row, UInt32* posValid, UInt32 *nrOfPoints, UInt32 thresh, UInt32 width)
.lalign
mvcvFast9M2_asm:
.nowarn 9, 10
LSU0.LDIL i0 0x98 || LSU1.LDIH i0 0x00
IAU.SUB i19 i19 i0
LSU0.STO.64.L v24 , i19 , 0x00 || LSU1.STO.64.H v24, i19, 0x08
LSU0.STO.64.L v25 , i19 , 0x10 || LSU1.STO.64.H v25, i19, 0x18
LSU0.STO.64.L v26 , i19 , 0x20 || LSU1.STO.64.H v26, i19, 0x28
LSU0.STO.64.L v27 , i19 , 0x30 || LSU1.STO.64.H v27, i19, 0x38
LSU0.STO.64.L v28 , i19 , 0x40 || LSU1.STO.64.H v28, i19, 0x48
LSU0.STO.64.L v29 , i19 , 0x50 || LSU1.STO.64.H v29, i19, 0x58
LSU0.STO.64.L v30 , i19 , 0x60 || LSU1.STO.64.H v30, i19, 0x68
LSU0.STO.64.L v31 , i19 , 0x70 || LSU1.STO.64.H v31, i19, 0x78
LSU0.STO.32 i20 , i19, 0x80    || LSU1.STO.32 i21, i19, 0x84
LSU0.STO.32 i22 , i19, 0x88    || LSU1.STO.32 i23, i19, 0x8C
LSU0.STO.32 i24 , i19, 0x90    || LSU1.STO.32 i25, i19, 0x94

bru.bra fastExclude 	|| iau.shl i0, i14, 2 				||cmu.cpii i5, i17 					||lsu0.ldo.32 i1, i18, 0x00	||lsu1.ldo.32 i2, i18, 0x0C// score// alocate space for posValid

iau.sub i19, i19, i0    ||cmu.cpii i13, i16 				||lsu1.ldo.32 i3, i18, 0x18			||lsu0.ldil i17, fast9M2_posValid
lsu0.sto.32 i0 , i19, -8				||lsu1.ldih i17, fast9M2_posValid //||cmu.cpii i17, i19 
iau.sub i16, i19, 4		||cmu.cpiv.x32 v7.0, i30			||lsu0.ldil i4, fast9M2_FAST_Exclude     ||  lsu1.ldih i4, fast9M2_FAST_Exclude
cmu.cpiv.x32 v7.1, i4	||lsu0.ldil i4, returnfastExclude   ||  lsu1.ldih i4, returnfastExclude
cmu.cpiv.x32 v7.2, i4	//||lsu1.st.32 i12, i13
cmu.cpvi.x32 i30, v7.1
.lalign
returnfastExclude:



//                       i18            i17           i16                i15                 i14               i13   
//void fastBitFlag(UInt8** row, UInt32* posValid, UInt8* scores, UInt16 *cornerPositions, UInt32 thresh, UInt32 nrOfPoints)
cmu.cmz.i32 i12 
peu.pc1c NEQ || bru.bra fastBitFlag
peu.pc1c EQ || lsu1.st.32 i12, i5
peu.pc1c EQ || lsu1.st.32 i12, i13
lsu0.ldil i17, fast9M2_posValid||lsu1.ldih i17, fast9M2_posValid
cmu.cpii i16, i5 ||iau.sub i14, i15, 1
cmu.cpii i15, i13
cmu.cpii i13, i12
nop
returnfastBitFlag:

.lalign
exitFast9M2:
lsu0.ldo.32 i16, i19, -8 ||cmu.cpvi.x32 i30, v7.0
LSU0.LDIL i0 0x98 || LSU1.LDIH i0 0x00
nop 5
iau.add i19, i19, i16

LSU0.LDO.64.L v24 , i19 , 0x00 || LSU1.LDO.64.H v24, i19, 0x08
LSU0.LDO.64.L v25 , i19 , 0x10 || LSU1.LDO.64.H v25, i19, 0x18
LSU0.LDO.64.L v26 , i19 , 0x20 || LSU1.LDO.64.H v26, i19, 0x28
LSU0.LDO.64.L v27 , i19 , 0x30 || LSU1.LDO.64.H v27, i19, 0x38
LSU0.LDO.64.L v28 , i19 , 0x40 || LSU1.LDO.64.H v28, i19, 0x48
LSU0.LDO.64.L v29 , i19 , 0x50 || LSU1.LDO.64.H v29, i19, 0x58
LSU0.LDO.64.L v30 , i19 , 0x60 || LSU1.LDO.64.H v30, i19, 0x68
LSU0.LDO.64.L v31 , i19 , 0x70 || LSU1.LDO.64.H v31, i19, 0x78
LSU0.LDO.32 i20 , i19, 0x80    || LSU1.LDO.32 i21, i19, 0x84
LSU0.LDO.32 i22 , i19, 0x88    || LSU1.LDO.32 i23, i19, 0x8C
LSU0.LDO.32 i24 , i19, 0x90    || LSU1.LDO.32 i25, i19, 0x94 || IAU.ADD i19 i19 i0
bru.jmp i30
nop 6
//                       i18            i17             i16             i15              i14
//void fastExclude(UInt8** row, UInt32* posValid, UInt32 *nrOfPoints, UInt32 thresh, UInt32 width)
//{
//	UInt32 posx//
//	Int32 threshL = thresh & 0xFF//
//    *nrOfPoints = 0//
//
//	for (posx = 0// posx < width// posx++)
//	{
//		if (((adiff(row[3][posx], row[0][posx]) <= threshL) && (adiff(row[3][posx], row[6][posx]) <= threshL)) ||
//            ((adiff(row[3][posx], row[3][posx + 3]) <= threshL) && (adiff(row[3][posx], row[3][posx - 3]) <= threshL)))
//		{
//			continue//
//		}
//        else
//        {
//            posValid[*nrOfPoints] = posx//
//            (*nrOfPoints)++//
//        }
//    }
//}i5
//i4, , i6, i7, i8, i9, i10, i11, i12, i13
fastExclude:// v23, v22,  v17, v16, v15, v14, v13, v12, v11
LSU0.LDO.32 i22, i18, 0x8       ||LSU1.LDO.32 i23, i18, 0x10    // row[4], row[2]
LSU0.LDo.64.l v5, i2 , -8 		||LSU1.LDo.64.h v5, i2 , 0			|| iau.sub i24 i2 8
LSU0.LDo.64.l v6, i2 , 8		||LSU1.LDo.64.h v6, i2 , 16 		||iau.add i2, i2, 24
LSU0.LDI.64.l v0, i1 			||LSU1.LDI.64.l v4, i3
LSU0.LDI.64.h v0, i1 			||LSU1.LDI.64.h v4, i3
iau.sub i14, i14, 0x10 			||lsu0.ldil i12, 0x00 				||lsu1.ldil i0, 0x0044// load and intialized nr of points found to 0
peu.pc1i LTE 					||bru.bra returnfastExclude 		|| lsu0.st.32 i12, i16 // exit code
LSU0.LDI.128.u8.u16 v28, i22    || LSU1.LDI.128.u8.u16 v24, i24
LSU0.LDI.128.u8.u16 v30, i23    || LSU1.LDI.128.u8.u16 v25, i24
LSU0.LDI.128.u8.u16 v29, i22    || LSU1.LDI.128.u8.u16 v26, i24
LSU0.LDI.128.u8.u16 v31, i23    || LSU1.LDI.128.u8.u16 v27, i24
lsu0.ldil i0, possitionsOffsets ||lsu1.ldih i0, possitionsOffsets 	||CMU.CPIVR.x16 v17, i0 // clear meask, keep just lt position// offset load
LSU0.LDO.64.l v18, i0, 0x00 	||LSU1.LDO.64.h v18, i0, 0x08 
LSU0.LDO.64.l v19, i0, 0x10 	||LSU1.LDO.64.h v19, i0, 0x18		||iau.add i15, i15, 1
LSU0.LDO.64.l v20, i0, 0x20 	||LSU1.LDO.64.h v20, i0, 0x28		||CMU.CPIVR.x8 v22, i15 // put thereshold in a vector
LSU0.LDO.64.l v21, i0, 0x30 	||LSU1.LDO.64.h v21, i0, 0x38 		||iau.sub i0, i0, i0    || cmu.alignvec v24 v24 v25 14// width initialization with 0

//loop start 
vau.add.i16 v28 v28 v30 || LSU0.LDIL i21 fast9M2_conv || LSU1.LDIH i21 fast9M2_conv || cmu.alignvec v8 v25 v26 2
vau.add.i16 v29 v29 v31 || iau.sub i24 i24 16 || cmu.alignvec v9 v25 v26 14
cmu.alignvec v27 v26 v27 2

fast9M2_FAST_Exclude:
VAU.ADD.i16 v1, v24, v8   ||  LSU1.LDI.128.u8.u16 v24, i24
VAU.ADD.i16 v2, v27, v9   ||  LSU1.LDI.128.u8.u16 v25, i24
VAU.ADD.i16 v1, v1, v28   ||  LSU1.LDI.128.u8.u16 v26, i24
VAU.ADD.i16 v2, v2, v29   ||  LSU1.LDI.128.u8.u16 v27, i24
VAU.SHR.u16 v1, v1, 2
VAU.SHR.u16 v2, v2, 2    || iau.sub i24 i24 16
VAU.ADD.i16 v1, v1, v25
nop
VAu.ADD.i16 v2, v2, v26  || cmu.alignvec v24 v24 v25 14
VAU.SHR.u16 v1, v1, 1    || cmu.alignvec v30 v25 v26 2     ||  LSU1.LDI.128.u8.u16 v28, i22
VAU.SHR.u16 v2, v2, 1    || cmu.alignvec v31 v25 v26 14    ||  LSU1.LDI.128.u8.u16 v29, i22
cmu.alignvec v27 v26 v27 2     ||  LSU1.LDI.128.u8.u16 v30, i23  || LSU0.LDI.64.l v0, i1
cmu.vdilv.x8 v3 v2 v1 v2   || LSU1.LDI.128.u8.u16 v31, i23

VAU.ADIFF.u8 v8,   v2,  v0  ||cmu.alignvec v1, v5, v6, 5 	||LSU1.LDI.64.h v0, i1          || LSU0.STI.64.L v2 , i21
VAU.ADIFF.u8 v9,   v2,  v4	||cmu.alignvec v3, v5, v6, 11 	|| iau.subsu i14, i14, i0 		|| LSU1.LDI.64.l v6, i2 || LSU0.STI.64.H v2 , i21
CMU.CMVV.u8 v8,  v22 		||VAU.ADIFF.u8 v10,  v2,  v1 	||peu.pcix.EQ  0x20 			||lsu1.cp i30, v7.2     || lsu0.cp v8 v30
PEU.ANDACC         			|| CMU.CMVV.u8 v9,  v22  		||VAU.ADIFF.u8 v11,  v2,  v3  	||iau.add i14, i14, i0 	||LSU1.LDI.64.h v6, i2// abs diff calculate
PEU.PC16C.AND LT  			|| BRU.jmp i30        			|| IAU.add i0, i0, 0x10 		|| cmu.cpvv v10, v22 // ultra early rejection, invalidate the second branch
CMU.CPTI i9 C_CMU0	    	||LSU1.LD.64.l v4, i3  ||iau.incs i3, 0x08 || lsu0.cp v9 v31
CMU.CPTI i10 C_CMU1 		||LSU1.LDI.64.h v4, i3 			||lsu0.cp v5, v6       || vau.add.i16 v28 v28 v30
CMU.CMVV.u8 v10,  v22   	||lsu0.cp v16.2 i0 				||lsu1.st.32 i12, i16  || vau.add.i16 v29 v29 v31// exit code
PEU.ANDACC         			|| CMU.CMVV.u8 v11,  v22		||lsu1.cp v16.3 i0
PEU.PC16C.AND LT  			|| BRU.jmp i30        			|| IAU.add i0, i0, 0x10  // ultra early rejection
CMU.CPTI i7 C_CMU0  		||lsu0.cp v16.1 i0
__end_loop_ultra_early_exit1:
CMU.CPTI i8 C_CMU1 			||iau.or i7, i9, i7 			||lsu0.cp v16.0 i0 
iau.or i8, i10, i8 			||lsu0.cp v23.0, i7 // copy the values in vrf for faster procesing and create validation mask
lsu1.cp v23.1, i8			||vau.add.i32 v12, v18, v16 
cmu.cpvv.u8.u16 v23, v23
VAU.and v23, v23, v17 // keep jut lt position
__end_loop_ultra_early_exit2:
BRU.jmp i30  				||vau.shl.x16 v23, v23, 4
lsu0.ldil i6, 0x0 			|| lsu1.ldih i6, 0xFFFF 		||vau.add.i32 v13, v19, v16// mask for enable bits, we have 4 vau that will be wrote
CMU.VNZ.x8 i6, v23, 0 		||vau.add.i32 v14, v20, v16// create a bit mask from vrf
iau.not i6, i6 				||vau.add.i32 v15, v21, v16
iau.ones i4, i6   			||LSU0.STCV.x32 v12 i17 i6 0xC
iau.shl i4, i4, 2 			||sau.add.i32  i12, i12, i4 // increment number of values found
iau.add i17, i17, i4 		||sau.add.i32  i0, i0, 0x10
__end_loop_for_valid_points:
// loop end 




//                       i18            i17           i16                i15                 i14               i13   
//void fastBitFlag(UInt8** row, UInt32* posValid, UInt8* scores, UInt16 *cornerPositions, UInt32 thresh, UInt32 nrOfPoints)
//{
//    UInt32 itr//
//    UInt32 counter = 0//
//    UInt16 *position  = cornerPositions + 2//
//    UInt8 *scoreValue = (scores + 4)//
//    UInt16 bitFlag = 0x01FF//
//    UInt32 *adrForCounter1 = (UInt32*)scores//
//    UInt32 *adrForCounter2 = (UInt32*)cornerPositions//
//	for (itr = 0// itr < nrOfPoints// itr++)
//	{
//        UInt32 posx = posValid[itr]//
//		UInt8 origin = row[3][posx]//
//        UInt8 sample[16]//
//        Int32 loLimit = origin - thresh//
//		Int32 hiLimit = origin + thresh//
//        UInt32 i = 0//
//        UInt16 bitMaskLow = 0//
//        UInt16 bitMaskHi = 0//
//        Int32 hiScore = 0//
//        Int32 loScore = 0//
//        UInt8 hiCount = 0//
//        UInt8 loCount = 0//
//
//        UInt32 finalCounter = 0//
//        UInt16 bitMask      = 0//
//        UInt16 scoreFin     = 0//
//
//		sample[0] = row[0][posx - 1]//
//		sample[1] = row[0][posx]//
//		sample[2] = row[0][posx + 1]//
//		sample[3] = row[1][posx + 2]//
//		sample[4] = row[2][posx + 3]//
//		sample[5] = row[3][posx + 3]//
//		sample[6] = row[4][posx + 3]//
//		sample[7] = row[5][posx + 2]//
//		sample[8] = row[6][posx + 1]//
//		sample[9] = row[6][posx]//
//		sample[10] = row[6][posx - 1]//
//		sample[11] = row[5][posx - 2]//
//		sample[12] = row[4][posx - 3]//
//		sample[13] = row[3][posx - 3]//
//		sample[14] = row[2][posx - 3]//
//		sample[15] = row[1][posx - 2]//
//
//        for (i = 0// i<16// i++)
//		{
//			if (sample[i] > hiLimit)
//			{
//				bitMaskHi |= 1 << i//
//				hiScore += (sample[i] - hiLimit)//
//				hiCount++//
//			}
//			else
//			if (sample[i] < loLimit)
//			{
//				bitMaskLow |= (1 << i)//
//				loScore += (loLimit - sample[i])//
//				loCount++//
//			}
//
//		}
//
//        if (hiCount >loCount)
//		{
//			bitMask = bitMaskHi//
//			scoreFin = hiScore//
//		}
//		else
//		{
//            bitMask = bitMaskLow//
//			scoreFin = loScore//
//		}
//
//
//        finalCounter = 0//
//        bitFlag = 0x01FF//
//		for (i = 0// i<16// i++)
//		{
//            
//			if ((bitMask & bitFlag) == bitFlag) 
//            {
//                finalCounter++//
//            }
//			if (bitFlag & 0x8000) 
//            {
//                bitFlag = ((bitFlag & 0x7FFF) << 1) + 1// 
//            }
//            else 
//            {
//                bitFlag = bitFlag << 1//
//            }
//		}
//
//
//		if (finalCounter != 0)
//		{
//			*position = posx//
//			position++//
//			*scoreValue = (UInt8)(loScore >> 4)//
//            scoreValue++//
//            counter++//
//		}
//     }
//    *adrForCounter1 = counter//
//    *adrForCounter2 = counter//
//}
//i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13, i14, i15, i16, i17, i18, i19
.lalign
fastBitFlag:
// create comparison vectors // load inputline reference address                               //thereshold

LSU0.LDIL i11, 0x01FF ||LSU1.LDIH i11, 0x03FE

                               SAU.ROL.x16 i12, i11, 0x02   ||LSU0.LDO.64.l v18, i18, 0x00 	||LSU1.LDO.64.h v18, i18, 0x08 ||cmu.cpivr.x8 v17, i14
CMU.CPIV.x32 v2.0, i11      || SAU.ROL.x16 i11, i11, 0x04   ||LSU0.LDO.64.l v19, i18, 0x10 	||LSU1.LDO.64.h v19, i18, 0x18
CMU.CPIV.x32 v2.1, i12      || SAU.ROL.x16 i12, i12, 0x04   ||lsu0.ldil i9, 0x00 ||lsu1.ldi.32r v20, i17
CMU.CPIV.x32 v2.2, i11      || SAU.ROL.x16 i11, i11, 0x04   ||LSU0.LDIL i14, fast9M2BitFlag_ret_label ||LSU1.LDIH i14, fast9M2BitFlag_ret_label// en loop label load
CMU.CPIV.x32 v2.3, i12      || SAU.ROL.x16 i12, i12, 0x04   
CMU.CPIV.x32 v3.0, i11      || SAU.ROL.x16 i11, i11, 0x04   
CMU.CPIV.x32 v3.1, i12      || SAU.ROL.x16 i12, i12, 0x04   
CMU.CPIV.x32 v3.2, i11     ||lsu0.cp  i7, i15 ||iau.add i15, i15, 0x04// score 
CMU.CPIV.x32 v3.3, i12   ||lsu0.cp i12, i16 ||iau.add i16, i16, 0x04// score
LSU0.LDIL i21 fast9M2_conv || LSU1.LDIH i21 fast9M2_conv
vau.iadds.u32 v0, v20, v18
vau.iadds.u32 v1, v20, v19
lsu0.cp i22 v20.0
iau.add i22 i21 i22
lsu0.ld.8 i22 i22
cmu.cpvi.x32 i0, v0.0	||lsu0.ldi.32r v20, i17 ||lsu1.cp i18, v20.0
lsu1.cp i0, v0.1	||lsu0.ldo.32 i1, i0, -1 	||cmu.cpiv v7.3, i18
lsu1.cp i0, v0.2	||lsu0.ldo.64 i1, i0, -2 
lsu1.cp i0, v0.3	||lsu0.ldo.64 i1, i0, -3 
lsu1.cp i0, v1.0	||lsu0.ldo.64 i1, i0, -3 
lsu1.cp i0, v1.1	||lsu0.ldo.64 i1, i0, -3 
lsu1.cp i0, v1.2	||lsu0.ldo.64 i1, i0, -2 
lsu0.ldo.32 i1, i0, -1 						||vau.iadds.u32 v0, v20, v18//3/3
cmu.vszm.byte i3, i1, [Z012] 				||vau.iadds.u32 v1, v20, v19
IAU.FINS i3, i1, 0x18, 0x08 || SAU.SWZ i4, i2, [0000] || PEU.PSEN4.BYTE 0x8
IAU.FINS i3, i1, 0x00, 0x08 || SAU.SWZ i4, i2, [2222] || PEU.PSEN4.BYTE 0x4	||CMU.CPIV.x32 v12.2, i3
IAU.FINS i3, i1, 0x08, 0x08 || SAU.SWZ i4, i2, [2222] || PEU.PSEN4.BYTE 0x2	||cmu.vszm.byte i0, i1, [ZZZ3]
IAU.FINS i3, i1, 0x10, 0x08 || SAU.SWZ i4, i2, [2222] || PEU.PSEN4.BYTE 0x1 ||CMU.CPIVR.x8 v16, i22
IAU.FINS i3, i1, 0x18, 0x08 || SAU.SWZ i4, i2, [0000] || PEU.PSEN4.BYTE 0x8 ||CMU.CPIV.x32 v12.1, i4 
                               SAU.SWZ i4, i1, [3210] || PEU.PSEN4.BYTE 0x7	||lsu1.cp v12.3, i3  || lsu0.cp i22 v20.0
VAU.ISUBS.u8 v14, v16, v17  || iau.add i22 i21 i22    ||lsu1.cp i18, v7.3// loLimit
lsu1.cp v12.0, i4 ||VAU.IADDS.u8 v15, v16, v17 || lsu0.ld.8 i23 i22// hiLimit

// loop start 
.lalign
fast9M2BitFlag_start_loop:
bru.rpl i14, i13, fast9M2BitFlag_end_loop || cmu.cpvi.x32 i0, v0.0						||lsu1.ldi.32r v20, i17 || lsu0.ld.8 i23 i22
lsu1.cp i0, v0.1			||lsu0.ldo.32 i1, i0, -1    ||cmu.vszm.word v7, v20 [0DDD] 	||VAU.ISUBS.u8 v10, v14, v12 	||iau.sub i10, i10, i10// LocalLoLimit
lsu1.cp i0, v0.2			||lsu0.ldo.64 i1, i0, -2 	||VAU.ISUBS.u8 v11, v12, v15 	||iau.sub i11, i11, i11// LocalHiLimit
lsu1.cp i0, v0.3			||lsu0.ldo.64 i1, i0, -3 	||cmu.vnz.x8 i10, v10, 0	   	||SAU.SUMX.u8 I8, v10 // score lo 
lsu1.cp i0, v1.0			||lsu0.ldo.64 i1, i0, -3 	||cmu.vnz.x8 i11, v11, 0	   	||IAU.ONES  i5, i10  
lsu1.cp i0, v1.1			||lsu0.ldo.64 i1, i0, -3 	||IAU.ONES  i6, i11  			||CMU.CPIVR.x16 v8, i10 	   
lsu1.cp i0, v1.2			||lsu0.ldo.64 i1, i0, -2 	||IAU.SUBSU i6, i6, i5
lsu0.ldo.32 i1, i0, -1 		||vau.iadds.u32 v0, v20, v18||PEU.PCIX.NEQ 0x06 			||CMU.CPIVR.x16 v8, i11          ||SAU.SUMX.u8 I8, v11 // score hi
cmu.vszm.byte i3, i1, [Z012]||vau.iadds.u32 v1, v20, v19
IAU.FINS i3, i1, 0x18, 0x08 ||cmu.vszm.byte i4, i2, [0DDD]
IAU.FINS i3, i1, 0x00, 0x08 ||cmu.vszm.byte i4, i2, [D2DD] 	|| lsu1.cp v12.2, i3      ||VAU.AND v5, v8, v2
fast9M2BitFlag_ret_label:

IAU.FINS i3, i1, 0x08, 0x08 ||SAU.SWZ i4, i2, [2222] 	    || PEU.PSEN4.BYTE 0x2	  ||cmu.vszm.byte i0, i1, [ZZZ3]

IAU.FINS i3, i1, 0x10, 0x08 ||SAU.SWZ i4, i2, [2222] 	    || PEU.PSEN4.BYTE 0x1 	  ||CMU.CPIVR.x8 v16, i23
IAU.FINS i3, i1, 0x18, 0x08 ||cmu.vszm.byte i4, i2, [0DDD]	||lsu1.cp 	v12.1, i4 	  ||VAU.AND v6, v8, v3	 // loLimit
lsu0.cp i4, i1 		||PEU.PL0EN4.BYTE 0x7	||lsu1.cp v12.3, i3 	  ||CMU.CMVV.u16 v2, v5  ||iau.shr.u32 i8, i8, 4  ||VAU.ISUBS.u8 v14, v16, v17 
lsu1.cp v12.0, i4 			||PEU.ORACC         		    || CMU.CMVV.u16 v3, v6    ||VAU.IADDS.u8 v15, v16, v17 || lsu0.cp i22 v20.0
PEU.PC8C.OR EQ    			|| IAU.ADD i9, i9, 0x1		|| LSU0.STI.8 i8, i16			||LSU1.STI.16 i18, i15// final predicate           //valid values            //score                     // position
iau.add i22 i21 i22 ||lsu1.cp i18, v7.3


fast9M2BitFlag_end_loop:
// loop end
bru.bra returnfastBitFlag
LSU0.ST.32 i9, i12
LSU0.ST.32 i9, i7
nop 4

.nowarnend
.end