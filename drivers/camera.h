// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __CAMERA_H
#define __CAMERA_H

#include <stdint.h>
#include <stdbool.h>
#include <libraw1394/raw1394.h>
#include <dc1394/control.h>
#include "plugins.h"

struct camera {
    char *name;
    uint64_t guid;
    dc1394camera_t *handle;
    dc1394video_mode_t mode;
    dc1394framerate_t framerate;
    int width;
    int height;
    bool color;
    int bytes__pixel;
    dc1394color_filter_t bayer_pattern;
    //for format 7:
    int top;
    int left;
    dc1394color_coding_t color_coding;
    int bytes_per_packet;
    int pan, shutter, gain, exposure, brightness;
    int frames;
    struct mapbuffer * sink;
};

struct plugin init(struct camera *cam);

#endif
