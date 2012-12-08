// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __MAPBUFFER_H
#define __MAPBUFFER_H

#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include "cor.h"

struct mapbuffer {
    unsigned char *buffer;
    dispatch_t *dispatch;
    packet_t *last_packet;
    size_t size;
    size_t free_ptr;
    size_t last_advise;
    size_t blocksize;
    size_t ahead;
    size_t behind;
    size_t waiting_on;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int fd;
    int indexed;
    int indexsize;
    bool mem_writable;
    bool file_writable;
    bool threaded;
    bool single_packet;
    const char *filename;
    uint64_t *index;
};

void mapbuffer_copy_packet(struct mapbuffer *mb, packet_t *p);
packet_t *mapbuffer_read_indexed(struct mapbuffer *mb, int *index);
packet_t *mapbuffer_alloc(struct mapbuffer *mb, enum packet_type type, uint32_t bytes);
void mapbuffer_enqueue(struct mapbuffer *mb, packet_t *p, uint64_t time);
packet_t *mapbuffer_alloc_unbounded(struct mapbuffer *mb, enum packet_type type);
void mapbuffer_enqueue_unbounded(struct mapbuffer *mb, packet_t *p, uint64_t time, uint32_t bytes);
packet_t *mapbuffer_read(struct mapbuffer *mb, uint64_t *index);
struct plugin mapbuffer_open(struct mapbuffer *mb);
void mapbuffer_close(struct mapbuffer *mb);
packet_t *mapbuffer_find_packet(struct mapbuffer *mb, int *index, uint64_t time, enum packet_type type);
void mapbuffer_init(struct mapbuffer *mb, uint32_t size_mb);

#endif
