// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __INBUFFER_H
#define __INBUFFER_H

#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include "cor.h"

struct inbuffer {
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
    char *filename;
    uint64_t *index;
};

void inbuffer_copy_packet(struct inbuffer *mb, packet_t *p);
packet_t *inbuffer_read_indexed(struct inbuffer *mb, int *index);
packet_t *inbuffer_alloc(struct inbuffer *mb, enum packet_type type, uint32_t bytes);
void inbuffer_enqueue(struct inbuffer *mb, packet_t *p, uint64_t time);
packet_t *inbuffer_alloc_unbounded(struct inbuffer *mb, enum packet_type type);
void inbuffer_enqueue_unbounded(struct inbuffer *mb, packet_t *p, uint64_t time, uint32_t bytes);
packet_t *inbuffer_read(struct inbuffer *mb, uint64_t *index);
struct plugin inbuffer_open(struct inbuffer *mb);
void inbuffer_close(struct inbuffer *mb);
packet_t *inbuffer_find_packet(struct inbuffer *mb, int *index, uint64_t time, enum packet_type type);
void inbuffer_init(struct inbuffer *mb, uint32_t size_mb);

#endif
