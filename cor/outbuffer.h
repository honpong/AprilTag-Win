// Created by Eagle Jones
// Copyright (c) 2013, RealityCap, Inc.
// All Rights Reserved.

#ifndef __OUTBUFFER_H
#define __OUTBUFFER_H

#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include "cor.h"

struct outbuffer {
    unsigned char *buffer;
    packet_t *last_packet;
    size_t size;
    size_t free_ptr;
    size_t waiting_on;
    size_t bytes_left;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int fd;
    const char *filename;
    char shm_filename[13];
    int shm_fd;
};

void outbuffer_copy_packet(struct outbuffer *mb, packet_t *p);
packet_t *outbuffer_alloc(struct outbuffer *mb, enum packet_type type, uint32_t bytes);
void outbuffer_enqueue(struct outbuffer *mb, packet_t *p, uint64_t time);
struct plugin outbuffer_open(struct outbuffer *mb);
void outbuffer_close(struct outbuffer *mb);

#endif
