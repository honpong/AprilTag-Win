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
.data .data.fast9ScoreCv_asm
.align 4
	possitionsOffsets:
        .int 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
    fast9u16score_posValid:
		.int 0
    fast9u16scoresInput:
        .int 0
.code .text.fast9ScoreCv_asm

mvcvfast9ScoreCv_asm:
.nowarn 10,11,12,22
IAU.SUB i19 i19 24 || lsu0.ldil i0, fast9u16score_posValid || lsu1.ldih i0, fast9u16score_posValid

iau.shl i1 i14 2
iau.add i1 i1 8
LSU1.ST.32 i13 i0 || IAU.ADD i13 i13 i1
lsu0.ldil i0, fast9u16scoresInput || lsu1.ldih i0, fast9u16scoresInput
LSU1.ST.32 i13 i0

LSU1.STO.32 i17 i19  4  || LSU0.STO.32 i18 i19 0
LSU1.STO.32 i15 i19 12  || LSU0.STO.32 i16 i19 8
LSU1.STO.32 i30 i19 20  || LSU0.STO.32 i14 i19 16
lsu0.ldil i30, fast9CvPoint_asm||lsu1.ldih i30, fast9CvPoint_asm
lsu0.ldil i17, fast9u16scoresInput || lsu1.ldih i17, fast9u16scoresInput
bru.swp i30 || lsu0.ld.32 i17, i17
nop 6
lsu1.ldo.32 i15, i19 8
lsu0.ldo.32 i18, i19  4 ||lsu1.ldo.32 i16, i19 12
lsu1.ldo.32  i30, i19  20
lsu0.ldil i17, fast9u16scoresInput || lsu1.ldih i17, fast9u16scoresInput
lsu0.ld.32 i17, i17
nop 6
LSU0.LDO.64.l v0, i17, 0x00 ||LSU1.LDO.64.h v0, i17, 0x08 || iau.incs i17, 0x10
nop
nop
lsu1.ldo.32 i15, i15 0x00
nop
nop
nop
iau.sub i0, i0, i0  ||vau.xor v7, v7, v7
cmu.vnz.x8 i0, v0, 0 ||vau.xor v8, v8, v8
CMU.CPIVR.x16 v1, i0  ||LSU0.LDIL i12, 0x1
cmu.cmz.i32 i15  ||VAU.AND v5, v1, v2
PEU.PC1C NEQ || bru.bra fastScore_asm1 ||VAU.AND v6, v1, v3  ||CMU.CPIVR.x16 v4, i12
lsu0.sto.32  i15, i18  0x00 || iau.incs i18, 4  ||sau.sub.i32 i16, i16, 1 ||CMU.CMVV.u16 v2, v5
//Disable PEU.PVV warning since we are using it on purpose

CMU.CMVV.u16 v3, v6 || PEU.PVV16 EQ || vau.or v7, v7, v4
PEU.PVV16 EQ || vau.or v8, v8, v4 ||LSU0.LDIL i4, 0x1

CMU.VDILV.x8 v9, v10, v7, v8 ||iau.sub i1, i1, i1  ||LSU0.LDIL i5, 0x0
cmu.vnz.x8 i1, v10, 0  ||LSU0.LDIL i11, 0x01FF//finalCounter
IAU.ONES I2, I1 ||LSU0.LDIL i9, 0x0
.lalign
fastScore_asm_return_from_fnc:

bru.jmp i30 ||IAU.add i19 i19 24
nop 6



//                   i18,         i17,         i16,       i15,             i14
//void fast9Point(UInt8** row, UInt8* score, UInt32 *base, UInt32 thresh, UInt32 width)
//-8    -4         i19,
//addTostackValue,nrOfPoints,lineForPosiiton width*4+4
//
//                       i18            i17             i16             i15              i14
//void fastExclude(UInt8** row, UInt32* posValid, UInt32 *nrOfPoints, UInt32 thresh, UInt32 width)
.lalign
fast9CvPoint_asm:

LSU0.LDIL i1 0x98 ||LSU1.LDIH i1 0x00 
IAU.SUB i19 i19 i1
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

cmu.cpii i5, i17 ||lsu0.ldil i17, fast9u16score_posValid || lsu1.ldih i17, fast9u16score_posValid

lsu0.ld.32 i17 i17
nop 6
bru.bra fastExclude1 	|| iau.shl i0, i14, 2 				||lsu0.ldo.32 i1, i18, 0x00	||lsu1.ldo.32 i2, i18, 0x0C// score// alocate space for posValid
iau.sub i19, i19, i0    ||cmu.cpii i13, i16 				||lsu1.ldo.32 i3, i18, 0x18 ||lsu0.ldo.32 i20, i18, 0x08//line 2
lsu0.sto.32 i0 , i19, -8                            ||lsu1.ldo.32 i21, i18, 0x10 //||cmu.cpii i17, i19
iau.sub i16, i19, 4		||cmu.cpiv.x32 v7.0, i30			||lsu0.ldil i4, fast9u16score_FAST_Exclude     ||  lsu1.ldih i4, fast9u16score_FAST_Exclude
cmu.cpiv.x32 v7.1, i4	||lsu0.ldil i4, returnfastExclude   ||  lsu1.ldih i4, returnfastExclude
cmu.cpiv.x32 v7.2, i4	//||lsu1.st.32 i12, i13
cmu.cpvi.x32 i30, v7.1
NOP
.lalign
returnfastExclude:



//lsu0.sto.32 i16, i17, 0x00 || lsu1.sto.32 i16, i17, 0x04//
//lsu0.sto.32 i16, i17, 0x08 || lsu1.sto.32 i16, i17, 0x0C


//                       i18            i17           i16                i15                 i14               i13
//void fastBitFlag(UInt8** row, UInt32* posValid, UInt8* scores, UInt16 *cornerPositions, UInt32 thresh, UInt32 nrOfPoints)
lsu1.ldil i17, fast9u16score_posValid
lsu1.ldih i17, fast9u16score_posValid
lsu1.ld.32 i17 i17
cmu.cmz.i32 i12
nop 4
// set the temp buffer to 0 at the end (the next loop will read them - it's important to have 0 there)
sau.xor i30, i30, i30 || iau.shl i16, i12, 2
iau.add i16, i16, i17
lsu0.sto.32 i30, i16, 0x00

peu.pc1c NEQ || bru.bra fastBitFlag1
peu.pc1c EQ || lsu1.st.32 i12, i5
peu.pc1c EQ || lsu1.st.32 i12, i13
nop
cmu.cpii i16, i5 ||iau.sub i14, i15, 1
cmu.cpii i15, i13
cmu.cpii i13, i12
nop
returnfastBitFlag:

.lalign
exitfast9u16score:

lsu0.ldo.32 i16, i19, -8 ||cmu.cpvi.x32 i30, v7.0
LSU0.LDIL i1 0x98 || LSU1.LDIH i1 0x00
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
LSU0.LDO.32 i24 , i19, 0x90    || LSU1.LDO.32 i25, i19, 0x94 || IAU.ADD i19 i19 i1
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
//		u8 val = (u8)(((u16)row[3][posx] + (((u16)row[2][posx] + (u16)row[4][posx] + (u16)row[3][posx - 1] + (u16)row[3][posx + 1]) >> 2)) >> 1);
//		if (((adiff(val, row[0][posx]) <= threshL) && (adiff(val, row[6][posx]) <= threshL)) ||
//            ((adiff(val, row[3][posx + 3]) <= threshL) && (adiff(val, row[3][posx - 3]) <= threshL)))
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
fastExclude1:// v23, v22,  v17, v16, v15, v14, v13, v12, v11
LSU0.LDo.64.l v5, i2 , -8 	||LSU1.LDo.64.h v5, i2 , 0

LSU0.LDo.64.l v6, i2 , 8		||LSU1.LDo.64.h v6, i2 , 16 		||iau.add i2, i2, 24
LSU0.LDI.64.l v0, i1, 			||LSU1.LDI.64.l v4, i3
LSU0.LDI.64.h v0, i1 			  ||LSU1.LDI.64.h v4, i3
LSU0.LDI.64.l v24, i20      ||LSU1.LDI.64.l v25, i21
LSU0.LDI.64.h v24, i20      ||LSU1.LDI.64.h v25, i21
iau.sub i14, i14, 0x10 			||lsu0.ldil i12, 0x00 				      ||lsu1.ldil i0, 0x0044// load and intialized nr of points found to 0
peu.pc1i LTE 					      ||bru.bra returnfastExclude 		    || lsu0.st.32 i12, i16 // exit code
lsu0.ldil i0, possitionsOffsets ||lsu1.ldih i0, possitionsOffsets 	||CMU.CPIVR.x16 v17, i0 // clear meask, keep just lt position// offset load
LSU0.LDO.64.l v18, i0, 0x00 	||LSU1.LDO.64.h v18, i0, 0x08                             
LSU0.LDO.64.l v19, i0, 0x10 	||LSU1.LDO.64.h v19, i0, 0x18		  ||iau.add i15, i15, 1   
LSU0.LDO.64.l v20, i0, 0x20 	||LSU1.LDO.64.h v20, i0, 0x28		  ||CMU.CPIVR.x8 v22, i15 // put thereshold in a vector
LSU0.LDO.64.l v21, i0, 0x30 	||LSU1.LDO.64.h v21, i0, 0x38 		||iau.sub i0, i0, i0    // width initialization with 0
//loop start
fast9u16score_FAST_Exclude:
cmu.alignvec v8, v5, v6, 7  || VAU.AVG.X8 v24, v24, v25 
cmu.alignvec v2, v5, v6, 9
VAU.AVG.X8   v2, v2, v8 
 nop
vau.avg.x8   v24, v24, v2
cmu.alignvec  v2,  v5, v6, 8
VAU.AVG.X8 v2, v24, v2  ||LSU0.LDI.64.l v24, i20  ||LSU1.LDI.64.l v25, i21 
                          LSU0.LDI.64.h v24, i20  ||LSU1.LDI.64.h v25, i21
                                                              LSU1.LDI.64.l v0, i1
VAU.ADIFF.u8 v8,   v2,  v0  ||cmu.alignvec v1, v5, v6, 5 	  ||LSU1.LDI.64.h v0, i1
VAU.ADIFF.u8 v9,   v2,  v4	||cmu.alignvec v3, v5, v6, 11 	||iau.subsu i14, i14, i0 		||LSU1.LDI.64.l v6, i2
VAU.ADIFF.u8 v10,  v2,  v1  ||CMU.CMVV.u8 v8,  v22   ||peu.pcix.EQ  0x20 			        ||lsu1.cp i30, v7.2
PEU.ANDACC         			||VAU.ADIFF.u8 v11,  v2,  v3 || CMU.CMVV.u8 v9,  v22  		 	    ||iau.add i14, i14, i0 	||LSU1.LDI.64.h v6, i2// abs diff calculate
PEU.PC16C.AND LT  			|| BRU.jmp i30        			 || IAU.add i0, i0, 0x10 		        ||cmu.cpvv v10, v22 // ultra early rejection, invalidate the second branch
CMU.CPTI i9 C_CMU0	    ||LSU1.LD.64.l v4, i3        ||iau.incs i3, 0x08 
CMU.CPTI i10 C_CMU1 		||LSU1.LDI.64.h v4, i3 			 ||lsu0.cp v5, v6
CMU.CMVV.u8 v10,  v22   ||lsu0.cp v16.2 i0 				   ||lsu1.st.32 i12, i16 // exit code
PEU.ANDACC         			|| CMU.CMVV.u8 v11,  v22		 ||lsu1.cp v16.3 i0
PEU.PC16C.AND LT  			|| BRU.jmp i30        			 || IAU.add i0, i0, 0x10  // ultra early rejection
CMU.CPTI i7 C_CMU0  		||lsu1.cp v16.1 i0
__end_loop_ultra_early_exit1:
CMU.CPTI i8 C_CMU1 			||iau.or i7, i9, i7 			||lsu0.cp v16.0 i0
iau.or i8, i10, i8 			||lsu0.cp v23.0, i7 // copy the values in vrf for faster procesing and create validation mask
lsu1.cp v23.1, i8			  ||vau.add.i32 v12, v18, v16
cmu.cpvv.u8.u16 v23, v23
VAU.and v23, v23, v17 // keep jut lt position
__end_loop_ultra_early_exit2:
BRU.jmp i30  				    ||vau.shl.x16 v23, v23, 4
lsu0.ldil i6, 0x0 			||lsu1.ldih i6, 0xFFFF 		||vau.add.i32 v13, v19, v16// mask for enable bits, we have 4 vau that will be wrote
CMU.VNZ.x8 i6, v23, 0 	||vau.add.i32 v14, v20, v16// create a bit mask from vrf

iau.not i6, i6 				  ||vau.add.i32 v15, v21, v16
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
fastBitFlag1:
// create comparison vectors // load inputline reference address                               //thereshold
LSU0.LDIL i11, 0x01FF ||LSU1.LDIH i11, 0x03FE
                               SAU.ROL.x16 i12, i11, 0x02   ||LSU0.LDO.64.l v18, i18, 0x00 	||LSU1.LDO.64.h v18, i18, 0x08 ||cmu.cpivr.x8 v17, i14
CMU.CPIV.x32 v2.0, i11      || SAU.ROL.x16 i11, i11, 0x04   ||LSU0.LDO.64.l v19, i18, 0x10 	||LSU1.LDO.64.h v19, i18, 0x18
CMU.CPIV.x32 v2.1, i12      || SAU.ROL.x16 i12, i12, 0x04   ||lsu0.ldil i9, 0x00 ||lsu1.ldi.32r v20, i17
CMU.CPIV.x32 v2.2, i11      || SAU.ROL.x16 i11, i11, 0x04   ||LSU0.LDIL i14, fast9u16scoreBitFlag_ret_label ||LSU1.LDIH i14, fast9u16scoreBitFlag_ret_label// en loop label load
CMU.CPIV.x32 v2.3, i12      || SAU.ROL.x16 i12, i12, 0x04
CMU.CPIV.x32 v3.0, i11      || SAU.ROL.x16 i11, i11, 0x04
CMU.CPIV.x32 v3.1, i12      || SAU.ROL.x16 i12, i12, 0x04
CMU.CPIV.x32 v3.2, i11     ||lsu0.cp  i7, i15 ||iau.add i15, i15, 0x04// score
CMU.CPIV.x32 v3.3, i12   ||lsu0.cp i12, i16 //||iau.add i16, i16, 0x04// score
vau.iadds.u32 v0, v20, v18
vau.iadds.u32 v1, v20, v19
cmu.cpvi.x32 i0, v0.0	||lsu0.ldi.32r v20, i17 ||lsu1.cp i18, v20.0
lsu1.cp i0, v0.1	||lsu0.ldo.32 i1, i0, -1 	||cmu.cpiv v7.3, i18
lsu1.cp i0, v0.2	||lsu0.ldo.64 i1, i0, -2
lsu1.cp i0, v0.3	||lsu0.ldo.64 i1, i0, -3
cmu.cpvi i0, v1.0	||lsu0.ldo.64 i1, i0, -3  || lsu1.cp i12 i0
cmu.cpvi i0, v1.1	||lsu0.ldo.64 i1, i0, -3  || lsu1.ldo.64.l v21 i12 -1 || iau.add i12 i0 0
lsu1.cp i0, v1.2	||lsu0.ldo.64 i1, i0, -2
lsu0.ldo.32 i1, i0, -1 						||vau.iadds.u32 v0, v20, v18 ||lsu1.ld.32 i5 i12 //3/3
cmu.vszm.byte i3, i1, [Z012] 				||vau.iadds.u32 v1, v20, v19
IAU.FINS i3, i1, 0x18, 0x08 || SAU.SWZ i4, i2, [0000] || PEU.PSEN4.BYTE 0x8 
IAU.FINS i3, i1, 0x00, 0x08 || SAU.SWZ i4, i2, [2222] || PEU.PSEN4.BYTE 0x4	||cmu.vszm.byte i6, i1, [ZZZ3] ||LSU1.CP v12.2, i3
IAU.FINS i3, i1, 0x08, 0x08 || SAU.SWZ i4, i2, [2222] || PEU.PSEN4.BYTE 0x2	
IAU.FINS i3, i1, 0x10, 0x08 || SAU.SWZ i4, i2, [2222] || PEU.PSEN4.BYTE 0x1 ||cmu.cpvi.x32 i12 v21.0  
IAU.FINS i3, i1, 0x18, 0x08 || SAU.SWZ i4, i2, [0000] || PEU.PSEN4.BYTE 0x8 ||cmu.cpiv.x32 v12.1, i4         
                               SAU.SWZ i4, i1, [3210] || PEU.PSEN4.BYTE 0x7	||lsu1.cp v12.3, i3            ||cmu.vszm.byte i10, i12, [Z2Z0] 
sau.sumx.u16 i10 i10        || cmu.vszm.byte i12, i12, [ZZZ1]
cmu.vszm.byte i0, i5, [ZZZ0]
iau.add i0, i0, i6
iau.add i0 i0 i10
iau.shr.u32 i0 i0 2
iau.add i0 i0 i12
iau.shr.u32 i0 i0 1
CMU.CPIVR.x8 v16, i0  
VAU.ISUBS.u8 v14, v16, v17 // loLimit
lsu1.cp v12.0, i4 ||VAU.IADDS.u8 v15, v16, v17 // hiLimit


// loop start
.lalign
fast9u16scoreBitFlag_start_loop:

bru.rpl i14, i13, fast9u16scoreBitFlag_end_loop  	||cmu.cpvi.x32 i0, v0.0						||lsu0.ldi.32r v20, i17 		||lsu1.cp i18, v7.3 ||VAU.ISUBS.u8 v10, v14, v12 
lsu1.cp i0, v0.1			||lsu0.ldo.32 i1, i0, -1    ||cmu.vszm.word v7, v20 [0DDD] 	||VAU.ISUBS.u8 v11, v12, v15 	||iau.sub i10, i10, i10// LocalLoLimit
lsu1.cp i0, v0.2			||lsu0.ldo.64 i1, i0, -2 	||cmu.vnz.x8 i10, v10, 0	        ||iau.sub i11, i11, i11// LocalHiLimit
cmu.cpvi i0, v0.3			||lsu0.ldo.64 i1, i0, -3 	||lsu1.ld.64.l v21, i0            ||SAU.SUMX.u8 I8, v10 // from line 2 ok
lsu1.cp i0, v1.0			||lsu0.ldo.64 i1, i0, -3 	||cmu.vnz.x8 i11, v11, 0	   	    ||IAU.ONES  i5, i10           ||sau.add.i32 i12 i0 0
lsu1.cp i0, v1.1			||lsu0.ldo.64 i1, i0, -3 	||IAU.ONES  i6, i11  			        ||CMU.CPIVR.x16 v8, i10
cmu.cpvi i0, v1.2			||lsu0.ldo.64 i1, i0, -2 	||IAU.SUBSU i6, i6, i5            ||lsu1.ldo.64.h v21 i12 -1

lsu0.ldo.32 i1, i0, -1 		||vau.iadds.u32 v0, v20, v18||PEU.PCIX.NEQ 0x26 			||CMU.CPIVR.x16 v8, i11     || lsu1.cp v10, v11 //||SAU.SUMX.u8 I8, v11
cmu.vszm.byte i3, i1, [Z012]||vau.iadds.u32 v1, v20, v19
IAU.FINS i3, i1, 0x18, 0x08 ||cmu.vszm.byte i4, i2, [0DDD]  ||sau.xor i12 i12 i12
IAU.FINS i3, i1, 0x00, 0x08 ||cmu.vszm.byte i4, i2, [D2DD] 	|| lsu1.cp v12.2, i3      ||VAU.AND v5, v8, v2  
IAU.FINS i3, i1, 0x08, 0x08 ||SAU.SWZ i4, i2, [2222] 	      || PEU.PSEN4.BYTE 0x2	      ||cmu.vszm.byte i0, i1, [ZZZ3]
IAU.FINS i3, i1, 0x10, 0x08 ||SAU.SWZ i4, i2, [2222] 	      || PEU.PSEN4.BYTE 0x1 	    ||cmu.vszm.byte i6, i1, [ZZZ3] //||cmu.cpiv v21.1 i1
IAU.FINS i3, i1, 0x18, 0x08 ||cmu.cpvi.X32 i10 v21.0        || lsu1.cp i11 v21.2 	   		// loLimit
cmu.vszm.byte i11, i11, [ZZZ2] ||SAU.SWZ i12, i11, [0000] 	|| PEU.PSEN4.BYTE 0x1   || iau.incs i0 -1
cmu.vszm.byte i10, i10, [ZZZ0] ||sau.add.i32 i11 i11 i12    
iau.add i10 i6 i10
iau.add i10 i11 i10
fast9u16scoreBitFlag_ret_label:
iau.shr.u32 i10 i10 2
iau.avg i0 i0 i10   ||cmu.vszm.byte i4, i2, [0DDD]	||lsu1.cp 	v12.1, i4 
cmu.cpivr.X8 v16 i0 ||VAU.AND v6, v8, v3
lsu0.cp i4, i1 	|| PEU.PL0EN4.BYTE 0x7	||lsu1.cp v12.3, i3 	  ||CMU.CMVV.u16 v2, v5  ||iau.shr.u32 i8, i8, 0  ||VAU.ISUBS.u8 v14, v16, v17
lsu1.cp v12.0, i4 			||PEU.ORACC         		    || CMU.CMVV.u16 v3, v6    ||VAU.IADDS.u8 v15, v16, v17
PEU.PC8C.OR EQ    			|| IAU.ADD i9, i9, 0x1		|| LSU0.STI.64.l v10, i16			||LSU1.STI.16 i18, i15// final predicate           //valid values
PEU.PC8C.OR EQ              ||      LSU0.STI.64.h v10, i16
//nop
fast9u16scoreBitFlag_end_loop:
// loop end
bru.bra returnfastBitFlag
nop //LSU0.ST.32 i9, i12
LSU0.ST.32 i9, i7
nop 4





//void fastScore_asmCver_asm(u16 *score, u8* scoresInput, unsigned int thresh, unsigned int nrOfPoints) {
//    unsigned int itr//
//    unsigned short bitMask//
//    u32 i//
//
//	for (itr = 0// itr < nrOfPoints// itr++)
//	{
//        //cmu.vnz
//        bitMask = 0//
//        for (i = 0// i<16// i++) {
//            if(scoresInput[i]) bitMask |= 1 << i//
//        }
//        // found first valid position
//        // 2 vau and after compare TRF registers will keep the mask,
//        // mask have to be continue, impossible to have gap
//        //VAU.AND v5, v8, v2
//        //VAU.AND v6, v8, v3
//        //CMU.CMVV.u16 v2, v5
//        //CMU.CMVV.u16 v3, v6 || cmu.cpti i0 C_CMU0
//        //                       cmu.cpti i1 C_CMU0
//        // iau.and i0 EQ (0b01000100010001000100010001000100)
//        // iau.and i1 EQ (0b01000100010001000100010001000100)
//        //
//        unsigned short finalCounter = 0//
//        unsigned short bitFlag = 0x01FF//
//		for (i = 0// i<16// i++)
//		{
//
//			if ((bitMask & bitFlag) == bitFlag)
//            {
//
//                finalCounter |= 1 << i//
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
//        u8 maxScore = 0//
//        u8 min = 0//
//        int  nrRep = ones(finalCounter)//
//        for(// nrRep// nrRep--) {
//           // IAU.BSF Id, Is
//            i = 0//
//            while ((finalCounter & (1<< i)) != (1<<i)) { i++//}
//            unsigned int bfs = i//
//
//            // rotate scores to
//            // vau.rol
//            vectorRotate(scoresInput, bfs)//
//
//            // minimum calculation 9 values
//            {
//                // min 1 1 instruction
//                u8 min1_1 = minimumCalc(scoresInput[0], scoresInput[1])//
//                u8 min1_2 = minimumCalc(scoresInput[2], scoresInput[3])//
//                u8 min1_3 = minimumCalc(scoresInput[4], scoresInput[5])//
//                u8 min1_4 = minimumCalc(scoresInput[6], scoresInput[7])//
//
//                // min 2
//                u8 min2_1 = minimumCalc(min1_1, min1_2)//
//                u8 min2_2 = minimumCalc(min1_3, min1_4)//
//
//                //min 3
//                u8 min3_1 = minimumCalc(min2_1, min2_2)//
//
//                //min 4 final
//                min = minimumCalc(min3_1, scoresInput[8])//
//            }
//            vectorRotate(scoresInput, 1)//
//            finalCounter = finalCounter >> (bfs+1)//
//            if(maxScore < min) maxScore = min//
//        }
//
//        scoresInput += 16//
//        *score = (u16)maxScore + thresh - 1//
//        score++//
//    }
//}

//                                i18           i17             i16                i15
//void fastScore_asm(u16 *score, u8* scoresInput, unsigned int thresh, unsigned int nrOfPoints) {
.lalign
fastScore_asm1:
.lalign
fast9ScoreCalc_line_loop:
IAU.BSF I3, I1
IAU.FINS i4, I3, 16, 8 ||SAU.ROL.x16  I6, I11, I3
iau.finsj i1, i5, i4
CMU.CMASK V11, I6, 0x00                    ||iau.sub i2, i2, 1
VAU.NOT V11, V11                            || cmu.cmz.i32 i2
VAU.IADDS.u8 v12, v0, v11
nop
CMU.VSZM.BYTE v13, v12 [ZZ32]
IAU.BSF I3, I1                                      ||CMU.MIN.u8 v12, v12, v13
IAU.FINS i4, I3, 16, 8 ||SAU.ROL.x16  I6, I11, I3                                     ||PEU.PCCX.EQ 0x30 || bru.bra fast9ScoreCalc_internal_finishL  ||LSU0.LDO.64.l v0, i17, 0x00 ||LSU1.LDO.64.h v0, i17, 0x08
iau.finsj i1, i5, i4                                ||CMU.VSZM.BYTE v13, v12 [ZZZ1]
CMU.CMASK V11, I6, 0x00                                                                             ||iau.sub i2, i2, 1
VAU.NOT V11, V11                                    ||CMU.MIN.u8 v12, v12, v13         ||PEU.PCCX.EQ 0x01 || iau.incs i17, 0x10
VAU.IADDS.u8 v12, v0, v11                                                                           || cmu.cmz.i32 i2
                                                    lsu0.cp i7 v12.0 ||lsu1.cp i8 v12.1 || cmu.cpvv v15, v12
CMU.VSZM.BYTE v13, v12 [ZZ32]                       ||lsu0.cp i10 v15.2 ||lsu1.cp i12 v15.3 ||IAU.MINU i7, i7, i8
.lalign
fast9ScoreCalc_internal_loop:
IAU.BSF I3, I1                                      ||CMU.MIN.u8 v12, v12, v13 //||sau.and i2, i2, i11
IAU.FINS i4, I3, 16, 8 ||SAU.ROL.x16  I6, I11, I3                               ||PEU.PCCX.NEQ 0x00 || bru.bra fast9ScoreCalc_internal_loop
iau.finsj i1, i5, i4                                ||CMU.VSZM.BYTE v13, v12 [ZZZ1] ||PEU.PCCX.EQ 0x30 ||LSU0.LDO.64.l v0, i17, 0x00 ||LSU1.LDO.64.h v0, i17, 0x08
CMU.CMASK V11, I6, 0x00                                                                         ||IAU.MINU i10, i10, i12  ||PEU.PCCX.EQ 0x02 || sau.add.i32 i17, i17, 0x10
VAU.NOT V11, V11                                    ||CMU.MIN.u8 v12, v12, v13                                      ||iau.sub i2, i2, 1
VAU.IADDS.u8 v12, v0, v11                                                                        ||IAU.MINU i7, i7, i10  ||cmu.cmz.i32 i2
                                                    lsu0.cp i7 v12.0 ||lsu1.cp i8 v12.1 || cmu.cpvv v15, v12 ||IAU.MAXU   i9, i9, i7
CMU.VSZM.BYTE v13, v12 [ZZ32]                       ||lsu0.cp i10 v15.2 ||lsu1.cp i12 v15.3 ||IAU.MINU i7, i7, i8
.lalign
fast9ScoreCalc_internal_finishL:
iau.sub i0, i0, i0  ||cmu.cpzv v7 0 ||sau.sub.i32 i15, i15, 1
IAU.MINU i10, i10, i12 ||cmu.vnz.x8 i0, v0, 0 ||vau.xor v8, v8, v8
IAU.MINU i7, i7, i10   ||CMU.CPIVR.x16 v1, i0
IAU.MAXU   i9, i9, i7  ||VAU.AND v5, v1, v2 ||cmu.cmz.i32 i15
VAU.AND v6, v1, v3   ||iau.add i9, i9, i16      ||PEU.PCCX.NEQ 0x00 || bru.bra fast9ScoreCalc_line_loop
CMU.CMVV.u16 v2, v5  ||lsu0.sti.8 i9, i18  ||LSU1.LDIL i9, 0x0 ||PEU.PCCX.EQ 0x00 ||bru.bra fastScore_asm_return_from_fnc
//Disable PEU.PVV warning since we are using it on purpose
CMU.CMVV.u16 v3, v6 || PEU.PVV16 EQ || vau.or v7, v7, v4
PEU.PVV16 EQ || vau.or v8, v8, v4

CMU.VDILV.x8 v9, v10, v7, v8 ||iau.sub i1, i1, i1
cmu.vnz.x8 i1, v10, 0 //finalCounter
IAU.ONES I2, I1
nop
.nowarnend
.end