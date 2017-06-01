#include "hw_breakpoints.h"
#include <stdio.h>

void insertHwBreakpoint(u8 breakpointNum, u32 address, u32 mask,
                        u8 instrFetchEnabled, u8 dataReadEnabled,
                        u8 dataWriteEnabled)
{
    // The HW logic for entering in breakpoints is:
    //     IF ((CURRENT_INSTR_OR_DATA_ADDRESS  &  mask)  ==  address)
    //         GENERATE_TRAP(0x0B);

    u32 asr_even_addr = (address & 0xFFFFFFFC) | (instrFetchEnabled & 1);
    u32 asr_odd_mask  = (mask & 0xFFFFFFFC) | ((dataReadEnabled & 1) << 1) |
                       (dataWriteEnabled & 1);

    // ASR pairs: %asr24/25, %asr26/27, %asr28/29 and %asr30/31

    switch(breakpointNum) {
        // !!! Write the asr even reg in the second step, to prevent triggering an instr. fetch bp
        case 0:
            asm volatile("wr %0, %%asr25 "
                         : /*regs out*/
                         : /*regs in*/ "r"(asr_odd_mask));
            asm volatile("wr %0, %%asr24 "
                         : /*regs out*/
                         : /*regs in*/ "r"(asr_even_addr));
            break;
        case 1:
            asm volatile("wr %0, %%asr27 "
                         : /*regs out*/
                         : /*regs in*/ "r"(asr_odd_mask));
            asm volatile("wr %0, %%asr26 "
                         : /*regs out*/
                         : /*regs in*/ "r"(asr_even_addr));
            break;
        case 2:
            asm volatile("wr %0, %%asr29 "
                         : /*regs out*/
                         : /*regs in*/ "r"(asr_odd_mask));
            asm volatile("wr %0, %%asr28 "
                         : /*regs out*/
                         : /*regs in*/ "r"(asr_even_addr));
            break;
        case 3:
            asm volatile("wr %0, %%asr31 "
                         : /*regs out*/
                         : /*regs in*/ "r"(asr_odd_mask));
            asm volatile("wr %0, %%asr30 "
                         : /*regs out*/
                         : /*regs in*/ "r"(asr_even_addr));
            break;
        default:
            printf("Error: only 4 HW breakpoints are available on Leon: 0, 1, "
                   "2, 3\n");
            break;
    }
}

void insertShaveHwBreakpoint(int shaveNum, void * breakAddr, u32 size)
{
    SET_REG_BITS_MASK(DCU_DCR(shaveNum), DCR_DBGE);  // Debug enable

    // Both data breakpoints are being used to work on range
    SET_REG_WORD(DCU_DBA0(shaveNum), breakAddr);
    SET_REG_WORD(DCU_DBA1(shaveNum), breakAddr + size - 1);

    int ctrlReg = 1 << 0     // Breakpoint enable
                  | 1 << 1   // Addr comparison enable
                  | 1 << 25  // Inclusive range
            ;
    SET_REG_WORD(DCU_DBC0(shaveNum), ctrlReg);  // Cfg only BP0
}
