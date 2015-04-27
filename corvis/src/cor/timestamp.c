// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#include "timestamp.h"

uint64_t cor_time_start = 0;

pthread_mutex_t cor_time_pb_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cor_time_pb_step = PTHREAD_COND_INITIALIZER;
int cor_time_pb_threads;
int cor_time_pb_threads_ready;
int cor_time_pb_dir = 1;
int64_t cor_time_pb_offset;
uint64_t cor_time_pb;
uint64_t cor_time_pb_candidate;
float cor_time_pb_scale;
bool cor_time_pb_real;
bool cor_time_pb_paused;
bool cor_time_pb_next_frame;

uint64_t cor_time_init()
{
    cor_time_start = cor_time() + cor_time_start;
    return cor_time_start;
}

uint64_t cor_time()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return ((uint64_t)tv.tv_sec*1000000 + tv.tv_usec) - cor_time_start;
}

void hiressleep(uint64_t delay)
{
    struct timeval tv = { .tv_sec = delay / 1000000, .tv_usec = delay % 1000000 };
    select(0, NULL, NULL, NULL, &tv);
}

int64_t cor_time_since_prev(uint64_t *prev)
{
    uint64_t old = *prev;
    *prev = cor_time();
    if (old == 0)
        return -1;
    return *prev - old;
}

struct timespec *cor_time_to_realtimespec(uint64_t time, struct timespec *ts)
{
    uint64_t realtime = time + cor_time_start;
    ts->tv_sec = realtime / 1000000ull;
    ts->tv_nsec = (realtime % 1000000ull) * 1000ull;
    return ts;
}

void cor_time_pb_control(bool pause, int dir)
{
    pthread_mutex_lock(&cor_time_pb_lock);
    int64_t rtime = cor_time();
    cor_time_pb_offset = rtime - cor_time_pb;
    cor_time_pb_paused = pause;
    cor_time_pb_dir = dir;
    pthread_cond_signal(&cor_time_pb_step);
    pthread_mutex_unlock(&cor_time_pb_lock);
}    

void cor_time_pb_pause()
{
    if(!cor_time_pb_paused) {
        cor_time_pb_control(true, 0);
        cor_time_pb_next_frame = false;
    } else {
        cor_time_pb_next_frame = true;
        cor_time_pb_unpause();
    }
}

void cor_time_pb_unpause()
{
    cor_time_pb_control(false, 1);
}

void cor_time_pb_forward()
{
    cor_time_pb_control(cor_time_pb_paused, 1);
}

void cor_time_pb_reverse()
{
    cor_time_pb_control(cor_time_pb_paused, -1);
}

void cor_time_pb_togglerealtime()
{
    cor_time_pb_real = !cor_time_pb_real;
    cor_time_pb_control(cor_time_pb_paused, cor_time_pb_dir);
}

void cor_time_pb_seek(int index)
{
    cor_time_pb_control(cor_time_pb_paused, cor_time_pb_dir);
}

void cor_time_pb_seekend()
{
    cor_time_pb_control(cor_time_pb_paused, cor_time_pb_dir);
}

void cor_time_pb_seekbeg()
{
    cor_time_pb_control(cor_time_pb_paused, cor_time_pb_dir);
}
