// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

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

static unsigned int pagesize;

void mapbuffer_close(struct mapbuffer *mb)
{
    if(!mb) return;
    if(mb->file_writable) msync(mb->buffer, mb->size, MS_SYNC);
    munmap(mb->buffer, mb->size);
    if(mb->file_writable) {
        if(ftruncate(mb->fd, mb->free_ptr)) {
            fprintf(stderr, "cor: ftruncate failed: %s\n", strerror(errno));
        }
        close(mb->fd);
    }
    if(mb->index) free(mb->index);
}

static void do_advise(struct mapbuffer *mb, uint64_t pos) {
    int64_t current = (pos / pagesize);
    current *= pagesize;
    if(mb->ahead && abs(current - mb->last_advise) > mb->blocksize) {
        int64_t needend = current + mb->ahead;
        if(needend > mb->size) needend = mb->size;
        int64_t needstart = current - mb->behind;
        if(needstart < 0) needstart = 0;
        int64_t dumpend = mb->last_advise + mb->ahead;
        if(dumpend > mb->size) dumpend = mb->size;
        int64_t dumpstart = mb->last_advise - mb->behind;
        if(dumpstart < 0) dumpstart = 0;
    
        if(dumpend > needstart && dumpend < needend) dumpend = needstart;
        if(dumpstart < needend && dumpstart > needstart) dumpstart = needend;
        if(mb->file_writable && dumpend) {
            //msync(mb->buffer + dumpstart, dumpend - dumpstart, MS_SYNC);
            //posix_fadvise(fd, dumpstart, dumpend - dumpstart, POSIX_FADV_DONTNEED);
            int rv = madvise(mb->buffer + dumpstart, dumpend - dumpstart, MADV_DONTNEED);
            if(rv) perror("mapbuffer: madvise (DONTNEED) failed");
        }
        if(dumpend) {
            int rv = madvise(mb->buffer + dumpstart, dumpend - dumpstart, MADV_DONTNEED);
            if(rv) perror("mapbuffer: madvise (DONTNEED) failed");
        }
        //int rv = madvise(mb->buffer + needstart, needend - needstart, MADV_WILLNEED);
        //int rv = posix_fadvise(mb->fd, needstart, needend - needstart, POSIX_FADV_WILLNEED);
        //if(rv) perror("mapbuffer: madvise (WILLNEED) failed");
        mb->last_advise = current;
    }
}

void *mapbuffer_start(struct mapbuffer *mb)
{
    uint64_t thread_pos = 0;
    while (mapbuffer_read(mb, &thread_pos)) {
        pthread_testcancel();
        if(mb->ahead) 
            do_advise(mb, thread_pos);
    }
    return NULL;
}

struct plugin mapbuffer_open(struct mapbuffer *mb)
{
    if(mb->indexsize)
        mb->index = malloc(mb->indexsize * sizeof(uint64_t));

    int prot = PROT_READ;
    int flags = MAP_NORESERVE;
#ifdef __APPLE__
    int mode = 0;
#else
    int mode = O_LARGEFILE;
#endif

    if(mb->mem_writable) {
        prot |= PROT_WRITE;
    }
    if(mb->file_writable) {
        mode |= O_CREAT | O_RDWR | O_TRUNC;
        flags |= MAP_SHARED;
    } else {
        mode |= O_RDONLY;
        flags |= MAP_PRIVATE;
    }
    if(mb->filename) {
        mb->fd = open(mb->filename, mode, 0644);
        if(mb->file_writable) {
            if(ftruncate(mb->fd, mb->size)) {
                fprintf(stderr, "cor: ftruncate failed: %s\n", strerror(errno));
            }
        }
        //if it's input we ignore the size specified by user
        struct stat stat_buf;
        fstat(mb->fd, &stat_buf);
        mb->size = stat_buf.st_size;
    } else {
#ifdef __APPLE__
        flags |= MAP_ANON;
#else
        flags |= MAP_ANONYMOUS;
#endif
        mb->fd = -1;
    }

    pagesize = getpagesize();
    mb->buffer = mmap(NULL, mb->size, prot, flags, mb->fd, 0);
    if(mb->buffer == (void *) -1) {
        fprintf(stderr, "buffer couldn't mmap %s, %llu %d %d %d: %s", mb->filename?mb->filename:"NULL", (unsigned long long)mb->size, prot, flags, mb->fd, strerror(errno));
        exit(EXIT_FAILURE);
    }
    madvise(mb->buffer, mb->size, MADV_SEQUENTIAL);
    mb->free_ptr = 0;
    pthread_mutex_init(&mb->mutex, NULL);
    pthread_cond_init(&mb->cond, NULL);

    return (struct plugin) {.data=mb, .start=mb->threaded?(void *(*)(void *))mapbuffer_start:0, .stop=(void (*)(void *))mapbuffer_close};
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
    if(mb->free_ptr + bytes > mb->size) {
        assert(0);
        cor_quit("out of buffer space\n");
        return 0;
    }

    packet_t *ptr;
    
    pthread_mutex_lock(&mb->mutex);

    size_t start = mb->free_ptr;
    if(!mb->single_packet)
        mb->free_ptr += bytes;
    ptr = (packet_t *)(mb->buffer + start);
    ptr->header.type = type;
    ptr->header.bytes = bytes;
    //don't initialize type and bytes. - the mmap dictates that they are zero, and we'll suck if the reader gets there first.
    pthread_mutex_unlock(&mb->mutex);
    
    return ptr;
}

//caution -- this is dangerous if there's more than one writer on a given buffer
packet_t * mapbuffer_alloc_unbounded(struct mapbuffer *mb, enum packet_type type)
{
    packet_t *ptr;
    pthread_mutex_lock(&mb->mutex);
    size_t start = mb->free_ptr;
    ptr = (packet_t *)(mb->buffer + start);
    ptr->header.type = type;
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
    //    if((uint8_t *)p - mb->buffer;
    pthread_mutex_unlock(&mb->mutex);
    mb->last_packet = p;
    if(mb->dispatch) dispatch(mb->dispatch, p);
}

void mapbuffer_enqueue_unbounded(struct mapbuffer *mb, packet_t *p, uint64_t time, uint32_t bytes)
{
    bytes = ((bytes + 7) & 0xfffffff8u);
    bytes += 16;
    if(mb->free_ptr + bytes > mb->size) {
        assert(0);
        cor_quit("out of buffer space\n");
        return;
    }

    pthread_mutex_lock(&mb->mutex);
    mb->free_ptr += bytes;
    p->header.time = time;
    p->header.bytes = bytes;
    if(mb->waiting_on == (uint8_t *)p - mb->buffer) {
        //somebody's waiting on us
        pthread_cond_broadcast(&mb->cond);
    }
    //    if((uint8_t *)p - mb->buffer;
    pthread_mutex_unlock(&mb->mutex);
    mb->last_packet = p;
    if(mb->dispatch) dispatch(mb->dispatch, p);
}

packet_t *mapbuffer_read(struct mapbuffer *mb, uint64_t *offset)
{
    if(*offset >= mb->size) return NULL;
    packet_t *ptr;

    pthread_mutex_lock(&mb->mutex);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &mb->mutex);
    ptr = (packet_t *)(mb->buffer + *offset);
 
    while(!ptr->header.time) {
        mb->waiting_on = *offset;
        pthread_cond_wait(&mb->cond, &mb->mutex);
    }

    *offset += ptr->header.bytes;
    pthread_cleanup_pop(1);

    return ptr;
}

packet_t *mapbuffer_read_indexed(struct mapbuffer *mb, int *index)
{
    uint64_t offset;
    packet_t *p;
    assert(mb->index);
    if(*index > mb->indexed) {
        *index = mb->indexed;
        return 0;
    }
    if(*index < 0) {
        *index = 0;
        return 0;
    }
    if(*index == mb->indexed) {
        offset = mb->index[*index];
        if((p = mapbuffer_read(mb, &offset))) {
            //another thread could have changed this...
            pthread_mutex_lock(&mb->mutex);
            if(*index == mb->indexed) {
                if(++mb->indexed == mb->indexsize) {
                    fprintf(stderr, "index size too small!\n");
                    --mb->indexed;
                } else {
                    mb->index[mb->indexed] = offset;
                }
            }
            pthread_mutex_unlock(&mb->mutex);
        }
        return p;
    }
    return (packet_t *)(mb->buffer + mb->index[*index]);
}

packet_t *mapbuffer_find_packet(struct mapbuffer *mb, int *index, uint64_t time, enum packet_type type)
{
    packet_t *p = mapbuffer_read_indexed(mb, index);
    int dir = 1;
    while(p && (p->header.time != time || p->header.type != type)) {
        ++*index;
        p = mapbuffer_read_indexed(mb, index);
    }
    return p;
}

void mapbuffer_init(struct mapbuffer *mb, uint32_t size_mb) {
    uint64_t MB = 1024*1024;
    mb->size = size_mb * MB;
    mb->ahead = 32 * MB;
    mb->behind = 256 * MB;
    mb->blocksize = 256 * 1024;
}
