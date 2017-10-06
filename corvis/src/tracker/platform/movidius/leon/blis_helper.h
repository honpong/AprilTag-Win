#ifndef _BLIS_HELPER_H_
#define _BLIS_HELPER_H_

#include "bli_config.h"
#include "swcShaveLoader.h"
#include "blis_defines.h"
#include <DrvShaveL2Cache.h>

#define BLIS_SHAVES 1

#ifdef SHAVE_PERFORMANCE_ONLY
extern double time_shave;
#endif

extern swcShaveUnit_t blis_shaves[BLIS_SHAVES];

extern u32 entryPointsSGEMM[BLIS_SHAVES];
extern u32 entryPointsSGEMMTRSM_LL[BLIS_SHAVES];
extern u32 entryPointsSGEMMTRSM_LU[BLIS_SHAVES];
extern u32 entryPointsSGEMMTRSM_RU[BLIS_SHAVES];
extern u32 entryPointsSGEMMTRSM_RL[BLIS_SHAVES];

#endif // _BLIS_HELPER_H_
