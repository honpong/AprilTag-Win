#include "blis_helper.h"

#ifdef SHAVE_PERFORMANCE_ONLY
double time_shave;
#endif


extern void* (blis4_startSGEMM);
extern void* (blis4_startSGEMMTRSM_LL);
extern void* (blis4_startSGEMMTRSM_LU);
extern void* (blis4_startSGEMMTRSM_RU);
extern void* (blis4_startSGEMMTRSM_RL);

// XXX - JR - This is a variable that blis library refers to it external
swcShaveUnit_t blis_shaves[BLIS_SHAVES] = {4};

u32 entryPointsSGEMM[BLIS_SHAVES] =
{
   (u32)&blis4_startSGEMM,
};

u32 entryPointsSGEMMTRSM_LL[BLIS_SHAVES] =
{
   (u32)&blis4_startSGEMMTRSM_LL,
};

u32 entryPointsSGEMMTRSM_LU[BLIS_SHAVES] =
{
   (u32)&blis4_startSGEMMTRSM_LU,
};

u32 entryPointsSGEMMTRSM_RU[BLIS_SHAVES] =
{
   (u32)&blis4_startSGEMMTRSM_RU,
};

u32 entryPointsSGEMMTRSM_RL[BLIS_SHAVES] =
{
   (u32)&blis4_startSGEMMTRSM_RL,
};
