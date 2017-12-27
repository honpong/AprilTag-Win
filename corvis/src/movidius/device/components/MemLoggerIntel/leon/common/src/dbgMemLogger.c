///
/// Implementation of the memory debug logger.
///

// 1: Includes
// ----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <mvMacros.h>
#include "dbgMemLoggerApiDefines.h"
#include "dbgMemLoggerApi.h"
#include "dbgLogEvents.h"
#include "registersMyriad.h"
#include "DrvRegUtils.h"
#include "swcLeonUtils.h"
#include "swcWhoAmI.h"
#include "rtems.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------
// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// Pointer to Buffer where all tracing info for memory logger component will be stored.
Node_t* memLogBuffer;
u32 bufferSize;

// sys clock divider shift (exponent) (from 600Mhz in TM2)
// in general events clocks time accuracy is 1/sys_clock * 2^divider_shift
// which translates for TM2 with divider shift of 7 into:
// 1/600Mhz * 2^7 = 0.21 usec
const u32 sysTick32ShiftRightNum = 7;

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// Global tracing level for the debug system
u32 traceBufHead = 1;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
static u64 getSysTicks64(void);
static u64 getSysTicks64Proc(swcProcessorType procType);
static u32 getSysTicks32WithShiftTrunc(void);
static u32 getSysTicks32ProcWithShiftTrunc(swcProcessorType procType);

// 6: Functions Implementation
// ----------------------------------------------------------------------------

/// Attach the user provided buffer to the pointer used by the Mem Logger
///
/// @param[in] u32* memPtr -- pointer to the user provided buffer
/// @param[in] u32 size -- the size of the above buffer
/// @return void
void dbgMemLogInit(u8* memPtr, u32 size)
{
	u32 tsLRT, tsLOS, interrupts = 0u;
	s32 tsDiff;

    memLogBuffer = (Node_t*)memPtr;
    bufferSize = size/sizeof(Node_t);

    memset(memLogBuffer, 0x0, size);

    // initialize the buffer header
    // first 4 bytes: offset of the real data
    memLogBuffer[0].time = 12;

    // second 4 bytes: tick offset (we use 0 for los and the offset between the counters for lrt
	if (swcWhoAmI() == PROCESS_LEON_OS)
	{
        memLogBuffer[0].id = 0;
	}
    else // LEON_RT
    {
    	// calcualte offset between LOS and LRT free running counter
    	// TODO: investigate accuracy requirements and adjust solution
    	rtems_interrupt_disable(interrupts);
    	tsLOS = getSysTicks32ProcWithShiftTrunc(PROCESS_LEON_OS);
    	tsLRT = getSysTicks32ProcWithShiftTrunc(PROCESS_LEON_RT);
    	rtems_interrupt_enable(interrupts);
    	tsDiff = tsLRT - tsLOS;
	    memLogBuffer[0].id = *((u32*)(&tsDiff));
    }

	// Indicate the divider from base sys clock (600Mhz for TM2)
    memLogBuffer[0].data = sysTick32ShiftRightNum;

    return;
}

/// Logs memory events by filling the dedicated buffer
/// The buffer is used as ring buffer
/// This function is passed as a parameter to the Init dbg API function of the Tracer
///
/// @param[in] Event ID
/// @param[in] Data field containing custom info
/// @param[in] Debug Level for the current eventId to determine if
///            the event would be traced or not.
/// @return void
void dbgMemLogEvent(Event_t eventId, u32 data)
{
	u32 interrupts = 0u;

	// disable interrupts to make 'atomic' timestamping and logging into the buffer
	rtems_interrupt_disable(interrupts);
    memLogBuffer[traceBufHead].time = getSysTicks32WithShiftTrunc();
    memLogBuffer[traceBufHead].id = eventId;
    memLogBuffer[traceBufHead].data = data;
    traceBufHead++;      
    if (bufferSize <= traceBufHead)
    {
        // Ring buffer is now full, so set its Head to the beginning
        traceBufHead = 1;
    }
    // restore interrupts
    rtems_interrupt_enable(interrupts);

    return;
}

// gets 64bit sys ticks for the given processor, only LOS and LRT are supported
// TODO: remove this function when such functionality is available on OSAL API
ATTR_UNUSED static u64 getSysTicks64(void)
{
	return getSysTicks64Proc(swcWhoAmI());
}

// gets 64bit sys ticks for the given processor, only LOS and LRT are supported
// TODO: remove this function when such functionality is available on OSAL API
static u64 getSysTicks64Proc(swcProcessorType procType)
{
	u32 timerBaseAddress = 0u, currentTicksH, currentTicksL;

	// set free running counter base address
	timerBaseAddress = 0;
	switch (procType) {
	case PROCESS_LEON_OS:
		timerBaseAddress = TIM0_BASE_ADR;
		break;
	case PROCESS_LEON_RT:
		timerBaseAddress = TIM1_BASE_ADR;
		break;
	default:
		printf("Unsupported processor type (%u) %s\n", procType, __FUNCTION__);
		swcLeonHalt();
	}

	// get 64 bit free running counter
	currentTicksH = GET_REG_WORD_VAL(timerBaseAddress + TIM_FREE_CNT1_OFFSET);  // On this read the low value is latched
    currentTicksL = GET_REG_WORD_VAL(timerBaseAddress + TIM_FREE_CNT0_OFFSET);  // previously latched low value read here

	return (((u64)currentTicksH << 32) | ((u64)currentTicksL));
}

// gets systick32 for the given processor and shift it right by sysTick32ShiftRightNum and then returns with it lower 32 bit
static u32 getSysTicks32WithShiftTrunc(void)
{
	return getSysTicks32ProcWithShiftTrunc(swcWhoAmI());
}

// gets systick32 and shift it right by sysTick32ShiftRightNum and then returns with it lower 32 bit
static u32 getSysTicks32ProcWithShiftTrunc(swcProcessorType procType)
{
	u32 tick32;
	u64 tick64;

	tick64 = getSysTicks64Proc(procType);
	tick32 = (tick64 >> sysTick32ShiftRightNum);

	return tick32;
}
