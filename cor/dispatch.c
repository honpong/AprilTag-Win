// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include "dispatch.h"
#include "mapbuffer.h"

struct dispatch_rewrite {
    int type;
    int64_t offset;
    struct dispatch_rewrite *next;
};

void dispatch_add_rewrite(dispatch_t *d, int type, uint64_t offset)
{
    assert(d);
    struct dispatch_rewrite *dr = calloc(1, sizeof(struct dispatch_rewrite));
    dr->type = type;
    dr->offset = offset;
    dr->next = d->rewrite;
    d->rewrite = dr;
}

static packet_t * dequeue_packet(dispatch_t *d)
{
    if(!d->reorder_size) return 0;
    //heapify
    int i = --d->reorder_size;
    int smallest = 0;
    while(smallest != i) {
        packet_t *tmp = d->reorder_queue[smallest];
        d->reorder_queue[smallest] = d->reorder_queue[i];
        d->reorder_queue[i] = tmp;        
        i = smallest;
        int l = 2 * i + 1;
        int r = 2 * i + 2;
        if((l < d->reorder_size) && (d->reorder_queue[l]->header.time < d->reorder_queue[smallest]->header.time))
            smallest = l;
        if((r < d->reorder_size) && (d->reorder_queue[r]->header.time < d->reorder_queue[smallest]->header.time))
            smallest = r;
    }
    return d->reorder_queue[d->reorder_size];
}

static void enqueue_packet(dispatch_t *d, packet_t *p)
{
    int i = d->reorder_size++;
    while((i > 0) && (d->reorder_queue[(i-1)/2]->header.time > p->header.time)) {
        d->reorder_queue[i] = d->reorder_queue[(i-1)/2];
        i = (i-1)/2;
    }
    d->reorder_queue[i] = p;
}

void dispatch(dispatch_t *d, packet_t *p)
{
    struct dispatch_rewrite *dr = d->rewrite;
    while(dr) {
        if(dr->type == p->header.type) {
            p->header.time += dr->offset;
            dr = 0;
        } else {
            dr = dr->next;
        }
    }
    if(d->reorder_depth) {
        enqueue_packet(d, p);
        if(d->reorder_size == d->reorder_depth)
            p = dequeue_packet(d);
        else
            return;
    }
    if(cor_time_pb_real) {
        int64_t rtime = cor_time();
        int64_t faketime = p->header.time + cor_time_pb_offset;
        if(cor_time_pb_scale) {
            faketime = p->header.time * cor_time_pb_scale + cor_time_pb_offset; // TODO: fix this so we can change timescale on the fly
        }
        if(faketime > rtime) {
            //future!
            struct timespec ts;
            pthread_mutex_lock(&cor_time_pb_lock);
            pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &cor_time_pb_lock);
            pthread_cond_timedwait(&cor_time_pb_step, &cor_time_pb_lock, cor_time_to_realtimespec(faketime, &ts));
            pthread_cleanup_pop(1);
        }
    }
    pthread_mutex_lock(&cor_time_pb_lock);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &cor_time_pb_lock);
    if(cor_time_pb_paused) {
        pthread_cond_wait(&cor_time_pb_step, &cor_time_pb_lock);
    }
    pthread_cleanup_pop(1);
    callback_dispatch(data, d->clients, p);
}

static void *start(dispatch_t *d)
{
    packet_t *p;
    uint64_t thread_pos = 0;
    while((p = mapbuffer_read(d->mb, &thread_pos))) {
        pthread_testcancel();
        dispatch(d, p);
    }
    return NULL;
}

struct plugin dispatch_init(dispatch_t *d)
{
    d->reorder_queue = calloc(d->reorder_depth, sizeof(*d->reorder_queue));
    d->reorder_size = 0;
    return (struct plugin) {.data = d, .start=d->threaded?(void *(*)(void *))start:0, .stop=0};
}
