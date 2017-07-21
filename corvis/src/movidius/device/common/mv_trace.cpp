#include "Trace.h"
#include "hw_breakpoints.h"

// Trace buffer; note that a speperate instace will be generated instanciated
// from LOS or LRT code
__attribute__((section(".ddr.bss"))) u8 traceBuffer[TRACE_BUFFER_SIZES];

void traceInit(void) { dbgMemLogInit(traceBuffer, sizeof(traceBuffer)); }
void setTraceBufferBP(u8 bpNum)
{
    extern void * memLogBuffer;
    insertHwBreakpoint(bpNum, (u32)(&memLogBuffer), ~(4 - 1), 1, 1, 1);
    for(int shv = 0; shv < 12; shv++)
        insertShaveHwBreakpoint(shv, &memLogBuffer, 4);
}
void unsetTraceBufferBP(u8 bpNum)
{
    insertHwBreakpoint(bpNum, 0, 0, 0, 0, 0);
    for(int shv = 0; shv < 12; shv++) insertShaveHwBreakpoint(shv, 0, 0);
}
