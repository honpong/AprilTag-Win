#ifndef _TRACE_H_
#define _TRACE_H_

#include <dbgMemLoggerApi.h>
#include <dbgLogEvents.h>
#ifndef EXTERNAL_RELEASE
#define USE_TRACE 1
#endif

#define SET_TRACE_BUFFER_BP 0

#define CORE_BASE (1 << 12)
#define SLAM_BASE (1 << 13)
#define CONTROLLERS_BASE (1 << 14)

#define BASIC_PROFILE (0 << 8)
#define COMPONENT_PROFILE (1 << 8)
#define DETAIL_PROFILE (2 << 8)
#define DEBUG_PROFILE (3 << 8)

// Trace EVENTS

#define EV_LOS_CONTEXT          (CORE_BASE | DEBUG_PROFILE | 0x0)
#define EV_LRT_CONTEXT          (CORE_BASE | DEBUG_PROFILE | 0x1)
#define EV_VGA_NOTIFY_FRAME     (CORE_BASE | DETAIL_PROFILE | 0x2)
#define EV_LOS_START            (CORE_BASE | DEBUG_PROFILE | 0x3)
#define EV_LRT_START            (CORE_BASE | DEBUG_PROFILE | 0x4)
#define EV_SF_DATA_CALL_FAST    (CORE_BASE | DEBUG_PROFILE | 0x5)
#define EV_SF_REC_IMAGE         (CORE_BASE | DETAIL_PROFILE | 0x6)
#define EV_SF_REC_GYRO          (CORE_BASE | DETAIL_PROFILE | 0x7)
#define EV_SF_REC_ACCEL         (CORE_BASE | DETAIL_PROFILE | 0x8)
#define EV_IMU_GYRO             (CORE_BASE | BASIC_PROFILE | 0x9)
#define EV_IMU_ACCEL            (CORE_BASE | BASIC_PROFILE | 0xA)
#define EV_SF_DATA_CALL_SLOW    (CORE_BASE | DETAIL_PROFILE | 0xB)
#define EV_SHAVE_TRACK          (CORE_BASE | COMPONENT_PROFILE | 0xC)
#define EV_SHAVE_DETECT         (CORE_BASE | COMPONENT_PROFILE | 0xD)
#define EV_CORE_IPC    			(CORE_BASE | DETAIL_PROFILE | 0xE)
#define EV_CORE_IPC_LRT   		(CORE_BASE | DETAIL_PROFILE | 0xF)
#define EV_CORE_USB_FRAMES_PROCESSING  (CORE_BASE | DETAIL_PROFILE | 0x10)
#define EV_CORE_USB_IMU_PROCESSING  (CORE_BASE | DETAIL_PROFILE | 0x11)
#define EV_CORE_USB_IMU_PROCESSING_WITHOUT_LOCK (CORE_BASE | DETAIL_PROFILE | 0x12)
#define EV_CORE_USB_6DOF_PROCESSING  (CORE_BASE | DETAIL_PROFILE | | 0x13)
#define EV_CORE_USB_WRITE   	(CORE_BASE | BASIC_PROFILE | 0x14)
#define EV_CORE_IMU_READING    	(CORE_BASE | DEBUG_PROFILE | 0x15)
#define EV_CORE_USB_6DOF_ACK    (CORE_BASE | DEBUG_PROFILE | 0x16)
#define EV_CENTRAL_ACCEL        (CORE_BASE | BASIC_PROFILE | 0x17)
#define EV_CENTRAL_GYRO         (CORE_BASE | BASIC_PROFILE | 0x18)
#define EV_CONTROLLER_POSE      (CORE_BASE | BASIC_PROFILE | 0x19)
#define EV_POOL_VIDEO_GET       (CORE_BASE | DEBUG_PROFILE | 0x1A)
#define EV_POOL_VIDEO_RETURN    (CORE_BASE | DEBUG_PROFILE | 0x1B)
#define EV_REPLAY_IO_READ       (CORE_BASE | DEBUG_PROFILE | 0x1C)
#define EV_NDDS_LOS_RECV        (CORE_BASE | DEBUG_PROFILE | 0x1D)
#define EV_NDDS_LRT_RECV        (CORE_BASE | DEBUG_PROFILE | 0x1E)
#define EV_NDDS_LOS_SEND        (CORE_BASE | DEBUG_PROFILE | 0x1F)
#define EV_NDDS_LRT_SEND        (CORE_BASE | DEBUG_PROFILE | 0x20)
#define EV_NDDS_LOS_PUT         (CORE_BASE | DEBUG_PROFILE | 0x21)
#define EV_NDDS_LRT_PUT         (CORE_BASE | DEBUG_PROFILE | 0x22)
#define EV_NDDS_LOS_DISPOSE     (CORE_BASE | DEBUG_PROFILE | 0x23)
#define EV_NDDS_LRT_DISPOSE     (CORE_BASE | DEBUG_PROFILE | 0x24)
#define EV_REPLAY_DELAY         (CORE_BASE | DETAIL_PROFILE | 0x25)
#define EV_SF_REC_STEREO         (CORE_BASE | DETAIL_PROFILE | 0x27)
#define EV_SF_REC_VELO          (CORE_BASE | DETAIL_PROFILE | 0x26)
#define EV_REPLAY_TRANSFER      (CORE_BASE | DETAIL_PROFILE | 0x28)

#define EVENT_GENERIC 0
#define EVENT_START_BITS (1 << 30)
#define EVENT_END_BITS   (2 << 30)

#define EVENT_TYPE_MASK (3 << 30)

#define LOG_CONTROL logState
#define TRACE_LEVEL traceLevel
#define TRACE_COMPONENTS traceComponents

extern int LOG_CONTROL;
extern int TRACE_LEVEL;
extern int TRACE_COMPONENTS;

#define LEVEL_MASK (0x3 << 8)
#define COMPONENTS_MASK (0x3FFFF << 10)

#if USE_TRACE == 1
#if SET_TRACE_BUFFER_BP == 1
    #define TRACE_EVENT(event, data) if (LOG_CONTROL) { unsetTraceBufferBP(2); dbgMemLogEvent((Event_t)(event), (uint32_t)(data)); setTraceBufferBP(2);}
#else
    #define TRACE_EVENT(event, data) if ((LOG_CONTROL) && ((event & LEVEL_MASK) <= TRACE_LEVEL) && (event & COMPONENTS_MASK & TRACE_COMPONENTS)) { dbgMemLogEvent((Event_t)(event), (uint32_t)(data)); }
#endif // SET_TRACE_BUFFER_BP
#define START_EVENT(event, data) TRACE_EVENT((((event) & (~EVENT_TYPE_MASK)) | EVENT_START_BITS), (data))
#define END_EVENT(event, data) TRACE_EVENT((((event) & (~EVENT_TYPE_MASK)) | EVENT_END_BITS), (data))
#define INIT_EVENTS() traceInit()


#define STOP_TRACING() if (1) { LOG_CONTROL = 0; }
#define START_TRACING() if (1) { LOG_CONTROL = 1; }

#else // don't USE_TRACE
#define TRACE_EVENT(event, data)
#define START_EVENT(event, data)
#define END_EVENT(event, data)
#define INIT_EVENTS()
#define STOP_LOGGING()
#define START_LOGGING()
#endif // USE_TRACE

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
