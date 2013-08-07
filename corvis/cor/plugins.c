// Copyright (c) 2005-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include "cor.h"

static struct plugin plugins[128];

static int nplugins = 0;

void plugins_register(struct plugin plugin)
{
    plugins[nplugins] = plugin;
    plugins[nplugins].thread = 0;
    ++nplugins;
}

void plugins_start()
{
    int res;
    pthread_attr_t attr;
    struct sched_param sched;
    if((res = pthread_attr_init(&attr))) {
        fprintf(stderr, "cor: couldn't initialize pthread attributes: %s\n", strerror(res));
        exit(EXIT_FAILURE);
    }
    if((res = pthread_attr_setschedpolicy(&attr, SCHED_FIFO))) {
        fprintf(stderr, "cor: couldn't set pthread sched policy: %s\n", strerror(res));
        exit(EXIT_FAILURE);
    }
    if((res = pthread_attr_setstacksize(&attr, 8*1024*1024))) {
        fprintf(stderr, "cor: couldn't set pthread stack size: %s\n", strerror(res));
        exit(EXIT_FAILURE);
    }   

    for(int i = 0; i < nplugins; ++i) {
        if(plugins[i].start && !plugins[i].thread) {
            sched.sched_priority = plugins[i].priority;
            if((res = pthread_create(&plugins[i].thread, &attr, plugins[i].start, plugins[i].data))) {
                fprintf(stderr, "cor: pthread create failed: %s\n", strerror(res));
                exit(EXIT_FAILURE);
            }
            if(plugins[i].priority) {
                if((res = pthread_setschedparam(plugins[i].thread, SCHED_FIFO, &sched))) {
                    fprintf(stderr, "cor: couldn't set pthread sched priority %d param: %s\n", plugins[i].priority, strerror(res));
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

void plugins_stop()
{
    int res;
    //stop plugins in reverse order
    for(int i = nplugins-1; i >= 0; --i) {
        if(plugins[i].thread) {
            pthread_cancel(plugins[i].thread);
            if((res = pthread_join(plugins[i].thread, NULL))) {
                fprintf(stderr, "cor: pthread_join failed: %s\n", strerror(res));
                exit(EXIT_FAILURE);
            }
            plugins[i].thread = 0;
        }
    }
    for(int i = nplugins-1; i >= 0; --i) {
        if(plugins[i].stop) {
            plugins[i].stop(plugins[i].data);
        }
    }
    nplugins = 0;
}

void plugins_clear()
{
    nplugins = 0;
}
