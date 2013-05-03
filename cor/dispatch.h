// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __DISPATCH_H
#define __DISPATCH_H

#include "packet.h"
#include "time.h"
#include "plugins.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
    void (*listener)(void *cookie, packet_t *p);
    void *cookie;
} callback_t;

struct dispatch_rewrite {
    int type;
    int64_t offset;
};

typedef struct {
    char *name;
    int reorder_depth;
    callback_t clients[10];
    int nclients;
    int reorder_size;
    packet_t *reorder_queue[100];
    struct dispatch_rewrite rewrite[10];
    struct mapbuffer *mb;
    uint64_t packets_dispatched;
    uint64_t bytes_dispatched;
    int num_rewrites;
    bool threaded;
    void (*progress_callback)(void *, float);
    void *progress_callback_object;
} dispatch_t;

inline static void dispatch_addclient(dispatch_t *d, void *cookie, void (*listener)(void *, packet_t *))
{
    if(!d) fprintf(stderr, "tried to subscribe to a non-existent dispatch!\n");
    assert(d);
    assert(d->nclients < 10);
    d->clients[d->nclients].cookie = cookie;
    d->clients[d->nclients].listener = listener;
    ++d->nclients;
}

void dispatch(dispatch_t *d, packet_t *p);
struct plugin dispatch_init(dispatch_t *d);
void dispatch_add_rewrite(dispatch_t *d, int type, uint64_t offset);

#endif
