// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#define _GNU_SOURCE
#include <linux/unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <libraw1394/raw1394.h>
#include <dc1394/control.h>
#include <signal.h>
#include <stdbool.h>
#include <poll.h>

#include "cor.h"
#include "debayer.h"
#include "mapbuffer.h"
#include "camera.h"

#define NUM_BUFFERS 4

dc1394_t *dc1394;

void stop(struct camera *cam)
{
    if(!cam) return;
    fprintf(stderr, "%d frames. ", cam->frames);
    dc1394_capture_stop(cam->handle);
    dc1394_video_set_transmission(cam->handle, DC1394_OFF);
    dc1394_camera_free(cam->handle);
    free(cam);
}

static void process_image(struct camera * cam, dc1394video_frame_t *frame)
{
    uint32_t width = frame->size[0];
    uint32_t height = frame->size[1];
    packet_t *buf = mapbuffer_alloc(cam->sink, packet_camera, 640*height + 16); // 16 bytes for pgm header
 
    sprintf((char *)buf->data, "P5 %4d %3d %d\n", 640, height, 255);
    bayer_to_gray(frame->image, width, height, buf->data+16, 640, height);

    //length of header
    buf->header.user = 16;
    static uint64_t last;
    uint64_t delta = frame->timestamp - last;
    if(delta > 36000)
        fprintf(stderr, "********** camera frame delta %lu\n", frame->timestamp - last);
    last = frame->timestamp;
    mapbuffer_enqueue(cam->sink, buf, frame->timestamp - cor_time_start);
}

void pgr_enable_timestamp (dc1394camera_t * cam)
{
    //0x12f8
    uint32_t quad = 0x80000001;
    dc1394error_t res = dc1394_set_control_register(cam, 0x12f8, quad);
    if (res != DC1394_SUCCESS) {
        fprintf(stderr, "couldn't read timestamp reg\n");
        return;
    }
    fprintf(stderr, "register was %x\n", quad);
}

uint64_t cycle_to_us(uint32_t cycle)
{
    return (cycle >> 25) * 1000000 + ((cycle >> 12) & 0x1fff) * 125 + (uint64_t)((cycle & 0xff0) / 24.567);
}

void *start(struct camera *cam)
{
    if(!cam) return NULL;
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);
    dc1394video_frame_t *frame;

    //flush any frames waiting for us
    struct pollfd pollfd;
    pollfd.events = POLLIN;
    pollfd.fd = dc1394_capture_get_fileno(cam->handle);
    while(poll(&pollfd, 1, 0) > 0) {
        dc1394_capture_dequeue(cam->handle, DC1394_CAPTURE_POLICY_POLL, &frame);
        fprintf(stderr, "camera dumped an old frame\n");
        dc1394_capture_enqueue(cam->handle, frame);
    }

    while(1) {
        pthread_testcancel();
        while(poll(&pollfd, 1, -1) < 1) {
            fprintf(stderr, "poll returned. testing for cancellation.\n");
            pthread_testcancel();
        }
        dc1394error_t res = dc1394_capture_dequeue(cam->handle, DC1394_CAPTURE_POLICY_WAIT, &frame);
        
	if (res != DC1394_SUCCESS) {
            fprintf(stderr, "camera: capture error %d", res);
            continue;
	}

        /*        uint32_t cycle_timer;
        uint64_t local_time;
        res = dc1394_read_cycle_timer(cam->handle, &cycle_timer, &local_time);
        
	if (res != DC1394_SUCCESS) {
            fprintf(stderr, "camera: couldn't read cycle time %d", res);
            continue;
	}
        uint64_t arrive_cycle = cycle_to_us(cycle_timer);
       
        uint32_t pgr = *(uint32_t *)frame->image;
        uint64_t capture_cycle = cycle_to_us(ntohl(pgr));
        uint64_t latency = arrive_cycle - capture_cycle - (local_time - frame->timestamp);
        fprintf(stderr, "time info: capt: %lu, cycle %lu, local %lu, pgr %lu, latency %lu\n", frame->timestamp, local_time, arrive_cycle, capture_cycle, latency);
        */
	process_image(cam, frame);
	//ttimestamp(frame->timestamp, camera_frame, "%s, time %lu, frame %d\n", cam->name, frame->timestamp, framenum);
        //callback_dispatch(sensor_data_camera, frame->timestamp, sensor_camera, cam, frame->image);

        res = dc1394_capture_enqueue(cam->handle, frame);
	if (res != DC1394_SUCCESS) {
            fprintf(stderr, "camera: capture error %d", res);
            continue;
	}
        ++cam->frames;
    }
    return 0;
}

void shutdown()
{
    dc1394_free(dc1394);
}

#define set_manual_value(cam, feat, val)				\
    do {                                                                \
        if (DC1394_SUCCESS != dc1394_feature_set_mode(cam, feat, DC1394_FEATURE_MODE_MANUAL)) { \
            fprintf(stderr, "Unable to set "#feat" mode\n");            \
            return (struct plugin) {};                                  \
        }                                                               \
        if (DC1394_SUCCESS != dc1394_feature_set_value(cam, feat, val)) { \
            fprintf(stderr, "Unable to set "#feat" value\n");           \
            return (struct plugin) {};                                  \
        }                                                               \
    } while(0)

struct plugin init(struct camera *cam)
{
    cam->mode = DC1394_VIDEO_MODE_FORMAT7_0;
    cam->framerate = DC1394_FRAMERATE_30;
    cam->color_coding = DC1394_COLOR_CODING_MONO8;

    if(!dc1394) {
        dc1394 = dc1394_new();
        atexit(shutdown);
    }
    cam->handle = dc1394_camera_new(dc1394, cam->guid);
    //shouldn't be doing this, but oh well...
    dc1394_reset_bus(cam->handle);
    if(!cam->handle) {
        fprintf(stderr, "camera: can't find camera: %s", cam->name);
        return (struct plugin) {};
    }
    //set speed as fast as possible
    if(cam->handle->bmode_capable) {
        if(dc1394_video_set_operation_mode(cam->handle, DC1394_OPERATION_MODE_1394B) == DC1394_SUCCESS) {
            dc1394_video_set_iso_speed(cam->handle, DC1394_ISO_SPEED_800);
        }
    } else {
        fprintf(stderr, "using 1394a (400mbps)\n");
        dc1394_video_set_iso_speed(cam->handle, DC1394_ISO_SPEED_400);
    }

    if(dc1394_video_set_mode(cam->handle, cam->mode) != DC1394_SUCCESS) {
        fprintf(stderr, "camera: couldn't set mode: %s", cam->name);
        return (struct plugin){};
    }
    if(cam->mode >= DC1394_VIDEO_MODE_FORMAT7_0) {
        if(dc1394_format7_set_roi(cam->handle, cam->mode, cam->color_coding, cam->bytes_per_packet, cam->left, cam->top, cam->width, cam->height) != DC1394_SUCCESS) {
            fprintf(stderr, "camera: couldn't set camera format 7 parameters: %s", cam->name);
            return (struct plugin){};
        }
    }
    if(dc1394_video_set_framerate(cam->handle, cam->framerate) != DC1394_SUCCESS) {
        fprintf(stderr, "camera: couldn't set camera framerate: %s", cam->name);
        return (struct plugin){};
    }
    set_manual_value(cam->handle, DC1394_FEATURE_PAN, cam->pan);
    set_manual_value(cam->handle, DC1394_FEATURE_SHUTTER, cam->shutter);
    set_manual_value(cam->handle, DC1394_FEATURE_GAIN, cam->gain);
    set_manual_value(cam->handle, DC1394_FEATURE_EXPOSURE, cam->exposure);
    set_manual_value(cam->handle, DC1394_FEATURE_BRIGHTNESS, cam->brightness);
    //pgr_enable_timestamp(cam->handle);
    if(dc1394_capture_setup(cam->handle, 10, DC1394_CAPTURE_FLAGS_DEFAULT) != DC1394_SUCCESS) {
        fprintf(stderr, "camera: unable to setup dma: %s", cam->name);
        return (struct plugin){};
    }
    /*have the camera start sending us data*/
    if (dc1394_video_set_transmission(cam->handle,DC1394_ON) != DC1394_SUCCESS) {
        error(0,0,"unable to start camera iso transmission");
        return (struct plugin){};
    }
    return (struct plugin) {.data=cam, .start=(void *(*)(void *))start, .stop=(void (*)(void *))stop};
}
