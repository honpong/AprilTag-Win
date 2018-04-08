/*
 * common_shave.h
 *
 *  Created on: Jun 4, 2017
 *      Author: ntuser
 */

#ifndef CORE_TARGET_SHAVE_STEREO_INITIALIZE_COMMON_SHAVE_H_
#define CORE_TARGET_SHAVE_STEREO_INITIALIZE_COMMON_SHAVE_H_

#include "svuCommonShave.h"


#if 0 // Configure debug prints
#include <stdio.h>
int shaveNum = scGetShaveNumber();
#define DEBUG_PRINTS
#define DPRINTF(...) if(shaveNum == 0) { printf(__VA_ARGS__); }
#else
#define DPRINTF(...)
#endif

unsigned short compute_mean7x7_from_pointer_array(int x_offset_center_from_pointer,int patch_win_half_width ,u8 **pPatch);
float score_match_from_pointer_array(u8 **pPatch1_pa, u8 **pPatch2_pa,int x1_offset_center_from_pointer , int x2_offset_center_from_pointer,int patch_win_half_width, unsigned short mean1, unsigned short mean2);

#endif /* CORE_TARGET_SHAVE_STEREO_INITIALIZE_COMMON_SHAVE_H_ */
