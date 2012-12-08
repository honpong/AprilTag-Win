// Created by Eagle Jones
// Copyright (c) 2012. RealityCap, Inc.
// All Rights Reserved.

#ifndef __RECOGNITION_H
#define __RECOGNITION_H

#include <map>
#include <vector>
#include <stdbool.h>
#include <stdint.h>
extern "C" {
#include <vl/sift.h>
#include <vl/ikmeans.h>
#include "cor.h"
}

using namespace std;

struct group {
    vl_sift_pix *grad;
    uint8_t *img;
    float orientation;
};

struct sift_descriptor {
    uint8_t d[128];
    sift_descriptor(uint8_t init[128]) {
        memcpy(d, init, 128);
    }
};

struct recognition {
    int maxkeys;
    int octaves;
    int levels;
    VlSiftFilt *sift;
    bool initialized;
    struct mapbuffer *imagebuf;
    int imageindex;
    struct mapbuffer *sink;
    map<uint64_t, struct group> groups;
    int width;
    int height;
    int32_t *dictionary;
    int dict_size;
    const char *dict_file;
    int numdescriptors;
    float maximum_feature_std_dev_percent;
};

struct sift_point {
    float x, y, scale, orientation;
};

#ifdef SWIG
%callback("%s_cb");
#endif
void sift_frame(void *handle, packet_t *p);
void recognition_packet(void *handle, packet_t *p);
#ifdef SWIG
%nocallback;
#endif

void recognition_init(struct recognition *f, int width, int height);
void recognition_build_dictionary(struct recognition *rec);
#endif
