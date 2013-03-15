// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef _PLUGINS_H
#define _PLUGINS_H

#include <pthread.h>

struct plugin {
    void *data;
    int priority;
    pthread_t thread;
    void * (*start)(void *);
    void (*stop)(void *);
};

//void plugins_register(void *data, void * (*start)(void *), void (*stop)(void *));
void plugins_register(struct plugin plugin);
void plugins_clear();

void plugins_start();
void plugins_stop();

#endif
