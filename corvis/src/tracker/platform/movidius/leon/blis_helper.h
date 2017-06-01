#ifndef _BLIS_HELPER_H_
#define _BLIS_HELPER_H_

#include "bli_config.h"
#include "swcShaveLoader.h"
#include "blis_defines.h"
#include <DrvShaveL2Cache.h>

#define MAX_SHAVES  1

#ifdef SHAVE_PERFORMANCE_ONLY
extern double time_shave;
#endif

extern swcShaveUnit_t listShaves[MAX_SHAVES];
extern int numberShaves;

/**
extern u32 entryPointsSGEMM[MAX_SHAVES];
extern u32 entryPointsSGEMMTRSM_LL[MAX_SHAVES];
extern u32 entryPointsSGEMMTRSM_LU[MAX_SHAVES];
extern u32 entryPointsSGEMMTRSM_RU[MAX_SHAVES];
extern u32 entryPointsSGEMMTRSM_RL[MAX_SHAVES];
**/
extern u32 entryPointsSGEMM[9];
extern u32 entryPointsSGEMMTRSM_LL[9];
extern u32 entryPointsSGEMMTRSM_LU[9];
extern u32 entryPointsSGEMMTRSM_RU[9];
extern u32 entryPointsSGEMMTRSM_RL[9];



#endif // _BLIS_HELPER_H_
