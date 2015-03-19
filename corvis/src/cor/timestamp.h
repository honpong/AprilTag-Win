// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __COR_TIME_H__
#define __COR_TIME_H__

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t cor_time_init();
uint64_t cor_time();
int64_t cor_time_since_prev(uint64_t *prev_us);
struct timespec *cor_time_to_realtimespec(uint64_t time, struct timespec *ts);
extern uint64_t cor_time_start;

extern pthread_mutex_t cor_time_pb_lock;
extern pthread_cond_t cor_time_pb_step;
extern int cor_time_pb_dir;
extern int64_t cor_time_pb_offset;
extern uint64_t cor_time_pb;
extern uint64_t cor_time_pb_candidate;
extern bool cor_time_pb_real;
extern float cor_time_pb_scale;
extern bool cor_time_pb_paused;
extern int cor_time_pb_threads;
extern int cor_time_pb_threads_ready;
extern bool cor_time_pb_next_frame;

void cor_time_pb_pause();
void cor_time_pb_unpause();
void cor_time_pb_forward();
void cor_time_pb_reverse();
void cor_time_pb_togglerealtime();
void cor_time_pb_seek(int index);
void cor_time_pb_seekend();
void cor_time_pb_seekbeg();

#ifdef __cplusplus
}
#endif

#endif /* __COR_TIME_PB_H__ */
