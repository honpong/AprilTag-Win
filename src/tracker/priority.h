#pragma once

#define PRIORITY_SLAM_FASTPATH   16
#define PRIORITY_SLAM_SLOWPATH    8
#define PRIORITY_SLAM_DETECT      6
#define PRIORITY_SLAM_ORB         4
#define PRIORITY_SLAM_RELOCALIZE  2
#define PRIORITY_SLAM_SAVE_MAP    2

#ifdef MYRIAD2
#include "thread_priorities.h"
#define set_priority(p) set_thread_priority(p)
#else
#define set_priority(p)
#endif

