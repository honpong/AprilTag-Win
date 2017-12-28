#pragma once

#define PRIORITY_SLAM_FASTPATH   12
#define PRIORITY_SLAM_SLOWPATH    8
#define PRIORITY_SLAM_DETECT      7
#define PRIORITY_SLAM_ORB         6
#define PRIORITY_SLAM_RELOCALIZE  5

#ifdef MYRIAD2
#include "thread_priorities.h"
#define set_priority(p) set_thread_priority((p), SCHED_FIFO)
#else
#define set_priority(p)
#endif

