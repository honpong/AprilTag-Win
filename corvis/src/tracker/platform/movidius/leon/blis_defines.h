///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     
///
#ifndef __DEFINES_H__
#define __DEFINES_H__

/* Shave allocation - use only 1 shave*/
#define MAX_SHAVES     1

/* PLEASE DON'T CHANGE THESE VALUES WITHOUT CHANGING CORRESPONDING DEPENDENTS */
/* MAX_M_ITER = (BLIS_DEFAULT_MC_S / 4) */
/* MAX_K      = BLIS_DEFAULT_KC_S       */
#define MAX_M_ITER       8
#define MAX_K          512

#define DDR_TO_DDRCACHE(_address) ((u8*)(((u32)_address)&0xF7FFFFFF))
#define ALIGNED(_value) __attribute__((aligned(_value)))
#define DDR_DATA      __attribute__((section(".ddr.data"))) ALIGNED(16)
#define DDR_BSS       __attribute__((section(".ddr.bss"))) ALIGNED(16)
#define DDR_LEON_HEAP __attribute__((section(".ddr.bss"))) ALIGNED(16)

typedef struct
{
   void *a;
   void *b;
   void *c;
   void *alpha;
   void *beta;
   int  k;
   int  n_iter;
   int  m_iter;
   int  n_left;
   int  m_left;
   int  rs_c;
   int  cs_c;
   int  rstep_a;
   int  cstep_b;
} kernelInfo_t;

#endif /*__DEFINES_H__*/
