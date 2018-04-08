#include "timestamp.h"

#include <DrvTimer.h>
#include <OsDrvCpr.h>

static uint32_t clkPerUs = 0;
uint64_t        TimestampNs()
{
    if(clkPerUs == 0) {
        OsDrvCprGetSysClockPerUs(&clkPerUs);
        printf("ClkPerUs: %lu\n", clkPerUs);
    }

    uint64_t nowClk;
    DrvTimerGetSystemTicks64((u64 *)&nowClk);
    return nowClk * 1000 / clkPerUs;
}
