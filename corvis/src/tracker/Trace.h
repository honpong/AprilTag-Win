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
#define SF_MUTEX_INIT_NORMAL    (SF_EVENT_BASE + 9)
#define SF_MUTEX_INIT_RECURSIVE (SF_EVENT_BASE + 10)
#define SF_MUTEX_ERROR          (SF_EVENT_BASE + 11)
#define SF_MUTEX_PRE_LOCK       (SF_EVENT_BASE + 12)
#define SF_MUTEX_POST_LOCK      (SF_EVENT_BASE + 13)
#define SF_MUTEX_UNLOCK         (SF_EVENT_BASE + 14)
#define SF_MUTEX_TRYLOCK        (SF_EVENT_BASE + 15)
#define SF_MUTEX_DESTROY        (SF_EVENT_BASE + 16)
#define SF_DETECT               (SF_EVENT_BASE + 17)
#define SF_TRACK                (SF_EVENT_BASE + 18)
#define SF_POP_QUEUE            (SF_EVENT_BASE + 19)
#define SF_DROP_LATE            (SF_EVENT_BASE + 20)
#define EV_SF_IMG2_STEREO_DETECT (SF_EVENT_BASE + 21)
#define EV_SF_IMG_STEREO_MEAS    (SF_EVENT_BASE + 22)
#define EV_SF_MATCH_FEATURES     (SF_EVENT_BASE + 23)
#define EV_FILTER_IMG_STEREO     (SF_EVENT_BASE + 25)
#define EV_DETECTING_GROUP_STEREO (SF_EVENT_BASE + 26)
#define EV_PROJECT_MOTION_COVARIANCE (SF_EVENT_BASE + 27)
