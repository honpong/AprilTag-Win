#include "blis_helper.h"

#ifdef SHAVE_PERFORMANCE_ONLY
double time_shave;
#endif
// XXX - JR - This is a variable that blis library refers to it external
int numberShaves = MAX_SHAVES;


extern void* (blis4_startSGEMM);
extern void* (blis4_startSGEMMTRSM_LL);
extern void* (blis4_startSGEMMTRSM_LU);
extern void* (blis4_startSGEMMTRSM_RU);
extern void* (blis4_startSGEMMTRSM_RL);

// XXX - JR - This is a variable that blis library refers to it external
swcShaveUnit_t listShaves[MAX_SHAVES] = {4};
/***
u32 entryPointsSGEMM[MAX_SHAVES] =
{
   (u32)&BLIS_GEMM5_startSGEMM,
};

u32 entryPointsSGEMMTRSM_LL[MAX_SHAVES] =
{
   (u32)&BLIS_GEMM5_startSGEMMTRSM_LL,
};

u32 entryPointsSGEMMTRSM_LU[MAX_SHAVES] =
{
   (u32)&BLIS_GEMM5_startSGEMMTRSM_LU
};

u32 entryPointsSGEMMTRSM_RU[MAX_SHAVES] =
{
   (u32)&BLIS_GEMM5_startSGEMMTRSM_RU
};

u32 entryPointsSGEMMTRSM_RL[MAX_SHAVES] =
{
   (u32)&BLIS_GEMM5_startSGEMMTRSM_RL
};
***/


// XXX - JR due to bug in blis library, it expects the entry points for shave N (0-11) to be
// located in the N position, regardless to the shave in use !
u32 entryPointsSGEMM[9] =
{
   (u32)&blis4_startSGEMM,
   (u32)&blis4_startSGEMM,
   (u32)&blis4_startSGEMM,
   (u32)&blis4_startSGEMM,
   (u32)&blis4_startSGEMM,
   (u32)&blis4_startSGEMM,
   (u32)&blis4_startSGEMM,
   (u32)&blis4_startSGEMM,
   (u32)&blis4_startSGEMM,
};

u32 entryPointsSGEMMTRSM_LL[9] =
{
   (u32)&blis4_startSGEMMTRSM_LL,
   (u32)&blis4_startSGEMMTRSM_LL,
   (u32)&blis4_startSGEMMTRSM_LL,
   (u32)&blis4_startSGEMMTRSM_LL,
   (u32)&blis4_startSGEMMTRSM_LL,
   (u32)&blis4_startSGEMMTRSM_LL,
   (u32)&blis4_startSGEMMTRSM_LL,
   (u32)&blis4_startSGEMMTRSM_LL,
   (u32)&blis4_startSGEMMTRSM_LL,
};

u32 entryPointsSGEMMTRSM_LU[9] =
{
   (u32)&blis4_startSGEMMTRSM_LU,
   (u32)&blis4_startSGEMMTRSM_LU,
   (u32)&blis4_startSGEMMTRSM_LU,
   (u32)&blis4_startSGEMMTRSM_LU,
   (u32)&blis4_startSGEMMTRSM_LU,
   (u32)&blis4_startSGEMMTRSM_LU,
   (u32)&blis4_startSGEMMTRSM_LU,
   (u32)&blis4_startSGEMMTRSM_LU,
   (u32)&blis4_startSGEMMTRSM_LU,
};

u32 entryPointsSGEMMTRSM_RU[9] =
{
   (u32)&blis4_startSGEMMTRSM_RU,
   (u32)&blis4_startSGEMMTRSM_RU,
   (u32)&blis4_startSGEMMTRSM_RU,
   (u32)&blis4_startSGEMMTRSM_RU,
   (u32)&blis4_startSGEMMTRSM_RU,
   (u32)&blis4_startSGEMMTRSM_RU,
   (u32)&blis4_startSGEMMTRSM_RU,
   (u32)&blis4_startSGEMMTRSM_RU,
   (u32)&blis4_startSGEMMTRSM_RU,
};

u32 entryPointsSGEMMTRSM_RL[9] =
{
   (u32)&blis4_startSGEMMTRSM_RL,
   (u32)&blis4_startSGEMMTRSM_RL,
   (u32)&blis4_startSGEMMTRSM_RL,
   (u32)&blis4_startSGEMMTRSM_RL,
   (u32)&blis4_startSGEMMTRSM_RL,
   (u32)&blis4_startSGEMMTRSM_RL,
   (u32)&blis4_startSGEMMTRSM_RL,
   (u32)&blis4_startSGEMMTRSM_RL,
   (u32)&blis4_startSGEMMTRSM_RL,
};
