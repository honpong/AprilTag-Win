#pragma once


#ifdef MYRIAD2
#include "mv_trace.h"
#else
#define START_EVENT(id, data)
#define END_EVENT(id, data)
#define TRACE_EVENT(id, data)
#define INIT_EVENTS() 
#endif

#define SF_EVENT_BASE (11000)
#define SF_IMAGE_MEAS           (SF_EVENT_BASE + 0)
#define SF_MINI_ACCEL_MEAS      (SF_EVENT_BASE + 1)
#define SF_MINI_GYRO_MEAS       (SF_EVENT_BASE + 2)
#define SF_ACCEL_MEAS           (SF_EVENT_BASE + 3)
#define SF_GYRO_MEAS            (SF_EVENT_BASE + 4)
#define SF_GEMM                 (SF_EVENT_BASE + 5)
#define SF_GEMM_HW              (SF_EVENT_BASE + 6)
#define SF_MSOLVE               (SF_EVENT_BASE + 7)
#define SF_MSOLVE_HW            (SF_EVENT_BASE + 8) 
#define SF_DETECT               (SF_EVENT_BASE + 17)
#define SF_TRACK                (SF_EVENT_BASE + 18)
#define SF_POP_QUEUE            (SF_EVENT_BASE + 19)
#define SF_DROP_LATE            (SF_EVENT_BASE + 20)
#define SF_STEREO_DETECT1       (SF_EVENT_BASE + 21)
#define SF_STEREO_MEAS          (SF_EVENT_BASE + 22)
#define SF_STEREO_MATCH         (SF_EVENT_BASE + 23)
#define SF_STEREO_RECEIVE       (SF_EVENT_BASE + 25)
#define SF_STEREO_DETECT2       (SF_EVENT_BASE + 26)
#define SF_PROJECT_MOTION_COVARIANCE (SF_EVENT_BASE + 27)
#define SF_FAST_PATH_CATCHUP    (SF_EVENT_BASE + 28)
#define SF_PROJECT_OBSERVATION_COVARIANCE (SF_EVENT_BASE + 29)
