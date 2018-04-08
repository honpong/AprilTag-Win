#ifndef _HW_BREAKPOINTS_
#define _HW_BREAKPOINTS_

#include <DrvSvu.h>
#include <DrvSvuDefines.h>
#include "registersMyriad.h"
#include "swcShaveLoader.h"

#ifdef __cplusplus
extern "C" {
#endif

// Leon breakpoint
// example: insertHwBreakpoint(3, break_address, ~(break_size-1), 0, 0, 1);
// To disable a breakpoint, use 0, 0 for address and mask
void insertHwBreakpoint(u8 breakpointNum, u32 address, u32 mask,
                        u8 instrFetchEnabled, u8 dataReadEnabled,
                        u8 dataWriteEnabled);
void insertShaveHwBreakpoint(int shaveNum, void * breakAddr, u32 size);

#ifdef __cplusplus
}
#endif

#endif
