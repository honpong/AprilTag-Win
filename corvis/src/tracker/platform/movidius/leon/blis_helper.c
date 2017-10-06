#include "blis_helper.h"

#ifdef SHAVE_PERFORMANCE_ONLY
double time_shave;
#endif


extern void* (blis0_startSGEMM);
extern void* (blis1_startSGEMM);
extern void* (blis2_startSGEMM);
extern void* (blis3_startSGEMM);
extern void* (blis0_startSGEMMTRSM_LL);
extern void* (blis1_startSGEMMTRSM_LL);
extern void* (blis2_startSGEMMTRSM_LL);
extern void* (blis3_startSGEMMTRSM_LL);
extern void* (blis0_startSGEMMTRSM_LU);
extern void* (blis1_startSGEMMTRSM_LU);
extern void* (blis2_startSGEMMTRSM_LU);
extern void* (blis3_startSGEMMTRSM_LU);
extern void* (blis0_startSGEMMTRSM_RU);
extern void* (blis1_startSGEMMTRSM_RU);
extern void* (blis2_startSGEMMTRSM_RU);
extern void* (blis3_startSGEMMTRSM_RU);
extern void* (blis0_startSGEMMTRSM_RL);
extern void* (blis1_startSGEMMTRSM_RL);
extern void* (blis2_startSGEMMTRSM_RL);
extern void* (blis3_startSGEMMTRSM_RL);

swcShaveUnit_t blis_shaves[BLIS_SHAVES] = {0,1,2,3};

u32 entryPointsSGEMM[BLIS_SHAVES] =
{
   (u32)&blis0_startSGEMM,
   (u32)&blis1_startSGEMM,
   (u32)&blis2_startSGEMM,
   (u32)&blis3_startSGEMM,
};

u32 entryPointsSGEMMTRSM_LL[BLIS_SHAVES] =
{
   (u32)&blis0_startSGEMMTRSM_LL,
   (u32)&blis1_startSGEMMTRSM_LL,
   (u32)&blis2_startSGEMMTRSM_LL,
   (u32)&blis3_startSGEMMTRSM_LL,
};

u32 entryPointsSGEMMTRSM_LU[BLIS_SHAVES] =
{
   (u32)&blis0_startSGEMMTRSM_LU,
   (u32)&blis1_startSGEMMTRSM_LU,
   (u32)&blis2_startSGEMMTRSM_LU,
   (u32)&blis3_startSGEMMTRSM_LU,
};

u32 entryPointsSGEMMTRSM_RU[BLIS_SHAVES] =
{
   (u32)&blis0_startSGEMMTRSM_RU,
   (u32)&blis1_startSGEMMTRSM_RU,
   (u32)&blis2_startSGEMMTRSM_RU,
   (u32)&blis3_startSGEMMTRSM_RU,
};

u32 entryPointsSGEMMTRSM_RL[BLIS_SHAVES] =
{
   (u32)&blis0_startSGEMMTRSM_RL,
   (u32)&blis1_startSGEMMTRSM_RL,
   (u32)&blis2_startSGEMMTRSM_RL,
   (u32)&blis3_startSGEMMTRSM_RL,
};
