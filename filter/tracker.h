#ifndef __TRACKER_H
#define __TRACKER_H

#include "model.h"

extern "C" {
#include "cor.h"
}

struct tracker {
    int width;
    int height;
    int maxfeats;
    int groupsize;
    int maxgroupsize;
    uint8_t *im1, *im2;
    struct mapbuffer *sink;
    void (* init)(int width, int height, int stride);
    feature_t (* track)(const uint8_t * im1, const uint8_t * im2, int currentx, int currenty, int x1, int y1, int x2, int y2);
};

#endif
