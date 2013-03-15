// Created by Eagle Jones
// Copyright (c) 2013, RealityCap, Inc.
// All Rights Reserved.

#define _GNU_SOURCE 1

#include <stdint.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>

#define PACKET_UNION
#include "cor.h"


#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

static unsigned int pagesize;

void mapbuffer_close(struct mapbuffer *mb)
{
    if(!mb) return;
    if(mb->filename) {
        close(mb->fd);
    }
    munmap(mb->buffer, mb->size);
    munmap(mb->buffer + mb->size, mb->size);
    close(mb->shm_fd);
    shm_unlink(mb->shm_filename);
    fprintf(stderr, "mapbuffer had %lld total bytes", mb->total_bytes);
}

//TODO: check into performance of this write thread. "write" may not be best way to dump our data
packet_t *mapbuffer_write(struct mapbuffer *mb, uint64_t *offset)
{
    if(*offset >= mb->size) *offset -= mb->size;
    packet_t *ptr;

    pthread_mutex_lock(&mb->mutex);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &mb->mutex);
    ptr = (packet_t *)(mb->buffer + *offset);
 
    while(!ptr->header.time) {
        mb->waiting_on = *offset;
        pthread_cond_wait(&mb->cond, &mb->mutex);
    }
    pthread_cleanup_pop(1);
    
    write(mb->fd, ptr, ptr->header.bytes);
    
    pthread_mutex_lock(&mb->mutex);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &mb->mutex);
    mb->bytes_left += ptr->header.bytes;

    *offset += ptr->header.bytes;
    pthread_cleanup_pop(1);

    pthread_cond_broadcast(&mb->cond); //wake up any threads that are waiting on free space

    return ptr;
}

void *mapbuffer_start(struct mapbuffer *mb)
{
    if(mb->replay) {
        packet_header_t header;
        while(1) {
            if(read(mb->fd, &header, sizeof(header)) != sizeof(header)) return NULL;
            packet_t *p = mapbuffer_alloc(mb, header.type, header.bytes - 16);
            p->header.user = header.user;
            if(read(mb->fd, p->data, header.bytes - 16) != header.bytes - 16) return NULL;
            mapbuffer_enqueue(mb, p, header.time);
            pthread_testcancel();
        }
    } else {
        uint64_t thread_pos = 0;
        while (mapbuffer_write(mb, &thread_pos)) {
            pthread_testcancel();
        }
    }
    return NULL;
}

struct plugin mapbuffer_open(struct mapbuffer *mb)
{

    pagesize = getpagesize();

    int prot = PROT_READ | PROT_WRITE;

    mb->buffer = mmap(NULL, mb->size * 2, prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if(mb->buffer == (void *) -1) {
        fprintf(stderr, "buffer couldn't mmap %s, %llu %d %d %d: %s", "double size buffer", (unsigned long long)mb->size, prot, MAP_SHARED, mb->shm_fd, strerror(errno));
        exit(EXIT_FAILURE);
    }

    sprintf(mb->shm_filename, "/corXXXXXX");
    mktemp(mb->shm_filename);
    mb->shm_fd = shm_open(mb->shm_filename, O_RDWR | O_CREAT | O_EXCL);
    if(mb->shm_fd == -1) {
        fprintf(stderr, "buffer couldn't open shared memory segment %s: %s\n", mb->shm_filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(ftruncate(mb->shm_fd, mb->size)) {
        fprintf(stderr, "buffer couldn't truncate shared memory segment to size %d: %s\n", mb->size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    void *rv = mmap(mb->buffer, mb->size, prot, MAP_FIXED | MAP_SHARED, mb->shm_fd, 0);
    if(rv == -1) {
        fprintf(stderr, "buffer couldn't mmap %s, %llu %d %d %d: %s", mb->shm_filename, (unsigned long long)mb->size, prot, MAP_FIXED | MAP_SHARED, mb->shm_fd, strerror(errno));
        exit(EXIT_FAILURE);
    }
    rv = mmap(mb->buffer + mb->size, mb->size, prot, MAP_FIXED | MAP_SHARED, mb->shm_fd, 0);
    if(rv == -1) {
        fprintf(stderr, "buffer couldn't mmap %s, %llu %d %d %d: %s", mb->shm_filename, (unsigned long long)mb->size, prot, MAP_FIXED | MAP_SHARED, mb->shm_fd, strerror(errno));
        exit(EXIT_FAILURE);
    }

    mb->total_bytes = 0;
    if(mb->filename) {
        if(mb->replay) {
            struct stat buf;
            mb->fd = open(mb->filename, O_RDONLY);
            fstat(mb->fd, &buf);
            mb->total_bytes = buf.st_size; 
        } else {
            mb->fd = open(mb->filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
        }
        if(mb->fd == -1) {
            fprintf(stderr, "buffer couldn't open backing data file %s: %s\n", mb->filename, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    mb->free_ptr = 0;
    mb->bytes_left = mb->size;
    pthread_mutex_init(&mb->mutex, NULL);
    pthread_cond_init(&mb->cond, NULL);

    return (struct plugin) {.data=mb, .start=(mb->filename?(void *(*)(void *))mapbuffer_start:NULL), .stop=(void (*)(void *))mapbuffer_close};
}

void mapbuffer_copy_packet(struct mapbuffer *mb, packet_t *p) {
    packet_t *p2 = mapbuffer_alloc(mb, p->header.type, p->header.bytes-16);
    memcpy(p2->data, p->data, p->header.bytes-16);
    p2->header.user = p->header.user;
    mapbuffer_enqueue(mb, p2, p->header.time);
}

packet_t *mapbuffer_alloc(struct mapbuffer *mb, enum packet_type type, uint32_t bytes)
{
    //add 7 and mask to pad to 8-byte boundary
    bytes = ((bytes + 7) & 0xfffffff8u);
    //header
    //TODO: fix all sizeof() calls on packets to subtract header size, or stop adding 16 here!
    bytes += 16;
    if(!mb->replay) {
        mb->total_bytes += bytes;
    }

    assert(mb->size > bytes * 2); //prevent deadlock condition from the buffer being too small

    packet_t *ptr;
    
    pthread_mutex_lock(&mb->mutex);

    while(bytes + 16 > mb->bytes_left) { //extra 16 bytes to clear out the next header (linked list next)
        //wait for the writing thread to clear some stuff
        pthread_cond_wait(&mb->cond, &mb->mutex);
    }
    
    size_t start = mb->free_ptr;

    mb->free_ptr += bytes;
    if(mb->free_ptr >= mb->size) mb->free_ptr -= mb->size;

    //clear out the packet header for the next one (linked list next)
    ptr = (packet_t *)(mb->buffer + mb->free_ptr);
    ptr->header.bytes = 0;
    ptr->header.type = 0;
    ptr->header.user = 0;
    ptr->header.time = 0;

    //only decrease available bytes if we are writing to a file - otherwise just feel free to overwrite old data
    if(mb->filename && !mb->replay) mb->bytes_left -= bytes;
    ptr = (packet_t *)(mb->buffer + start);
    ptr->header.type = type;
    ptr->header.bytes = bytes;
    
    pthread_mutex_unlock(&mb->mutex);
    
    return ptr;
}

void mapbuffer_enqueue(struct mapbuffer *mb, packet_t *p, uint64_t time)
{
    pthread_mutex_lock(&mb->mutex);
    p->header.time = time;
    if(mb->waiting_on == (uint8_t *)p - mb->buffer) {
        //somebody's waiting on us
        pthread_cond_broadcast(&mb->cond);
    }
    pthread_mutex_unlock(&mb->mutex);
    mb->last_packet = p;
    if(mb->dispatch) dispatch(mb->dispatch, p);
}

packet_t *mapbuffer_read(struct mapbuffer *mb, uint64_t *offset)
{
    packet_t *ptr;
    pthread_mutex_lock(&mb->mutex);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &mb->mutex);
    ptr = (packet_t *)(mb->buffer + *offset);
 
    while(!ptr->header.time) {
        mb->waiting_on = *offset;
        pthread_cond_wait(&mb->cond, &mb->mutex);
    }

    *offset += ptr->header.bytes;
    if(*offset >= mb->size) mb->free_ptr -= mb->size;
    pthread_cleanup_pop(1);

    return ptr;
}
