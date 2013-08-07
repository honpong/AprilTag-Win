// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <pthread.h>
#include "debug.h"

static pthread_mutex_t debug_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t debug_cond = PTHREAD_COND_INITIALIZER;

void debug_stop()
{
    pthread_mutex_lock(&debug_mutex);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &debug_mutex);
    pthread_cond_wait(&debug_cond, &debug_mutex);
    pthread_cleanup_pop(1);
}

void debug_continue()
{
    pthread_mutex_lock(&debug_mutex);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &debug_mutex);
    pthread_cond_broadcast(&debug_cond);
    pthread_cleanup_pop(1);
}
