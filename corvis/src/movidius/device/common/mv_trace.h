#ifndef _TRACE_H_
#define _TRACE_H_

#include <dbgLogEvents.h>
#include <dbgMemLoggerApi.h>

#define USE_TRACE 1
#define SET_TRACE_BUFFER_BP 0

// Trace EVENTS
#define TRACE_BASE 10000

#define EV_VGA_NOTIFY_FRAME \
    (TRACE_BASE + 4)  // VGA thread frame notify Data : frameCount
#define EV_LOS_START (TRACE_BASE + 13)
#define EV_LRT_START (TRACE_BASE + 14)
#define EV_SF_DATA_CALL_FAST (TRACE_BASE + 24)
#define EV_SF_REC_IMAGE (TRACE_BASE + 25)
#define EV_SF_REC_GYRO (TRACE_BASE + 26)
#define EV_SF_REC_ACCEL (TRACE_BASE + 27)
#define EV_IMU_GYRO (TRACE_BASE + 28)
#define EV_IMU_ACCEL (TRACE_BASE + 29)
#define EV_SF_DATA_CALL_SLOW (TRACE_BASE + 31)
#define EV_SHAVE_TRACK (TRACE_BASE + 32)
#define EV_SHAVE_DETECT (TRACE_BASE + 33)
#define EV_CORE_IPC (TRACE_BASE + 34)
#define EV_CORE_IPC_MP (TRACE_BASE + 35)
#define EV_CORE_USB_FRAMES_PROCESSING (TRACE_BASE + 36)
#define EV_CORE_USB_IMU_PROCESSING (TRACE_BASE + 37)
#define EV_CORE_USB_IMU_PROCESSING_WITHOUT_LOCK (TRACE_BASE + 38)
#define EV_CORE_USB_6DOF_PROCESSING (TRACE_BASE + 39)
#define EV_CORE_USB_WRITE (TRACE_BASE + 40)
#define EV_CORE_IMU_READING (TRACE_BASE + 41)
#define EV_CORE_USB_6DOF_ACK (TRACE_BASE + 42)
#define EV_SF_REC_STEREO (TRACE_BASE + 43)

#define EVENT_GENERIC 0
#define EVENT_START_BITS (1 << 30)
#define EVENT_END_BITS (2 << 30)

#define EVENT_TYPE_MASK (3 << 30)

#if USE_TRACE == 1
#if SET_TRACE_BUFFER_BP == 1
#define TRACE_EVENT(event, data)                            \
    if(1) {                                                 \
        unsetTraceBufferBP(2);                              \
        dbgMemLogEvent((Event_t)(event), (uint32_t)(data)); \
        setTraceBufferBP(2);                                \
    }
#else
#define TRACE_EVENT(event, data)                            \
    if(1) {                                                 \
        dbgMemLogEvent((Event_t)(event), (uint32_t)(data)); \
    }
#endif  // SET_TRACE_BUFFER_BP
#define START_EVENT(event, data) \
    TRACE_EVENT((((event) & (~EVENT_TYPE_MASK)) | EVENT_START_BITS), (data))
#define END_EVENT(event, data) \
    TRACE_EVENT((((event) & (~EVENT_TYPE_MASK)) | EVENT_END_BITS), (data))
#define INIT_EVENTS() traceInit()
#else  // don't USE_TRACE
#define TRACE_EVENT(event, data)
#define START_EVENT(event, data)
#define END_EVENT(event, data)
#define INIT_EVENTS()
#endif  // USE_TRACE

extern u8 traceBuffer[TRACE_BUFFER_SIZES];

#ifdef __cplusplus
extern "C" {
#endif
extern void traceInit(void);
extern void setTraceBufferBP(u8 bpNum);
extern void unsetTraceBufferBP(u8 bpNum);

#ifdef __cplusplus
}
#endif

#endif
