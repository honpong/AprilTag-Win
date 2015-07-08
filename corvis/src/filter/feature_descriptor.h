#ifndef __FEATURE_DESCRIPTOR_H
#define __FEATURE_DESCRIPTOR_H

extern "C" {
#include "../vlfeat-0.9.18/vl/liop.h"
}

#include <stdio.h>

const static int liop_spatial_bins = 6;
const static int liop_neighbors = 4;
const static int descriptor_size = liop_spatial_bins * 4*3*2*1; // change if liop_neighbors changes

void liop_compute_desc(VlLiopDesc * liop, const uint8_t * I, int stride, float center_x, float center_y, int side_length, float * patch, float * desc);

struct descriptor {
    float d[descriptor_size];
    uint8_t patch[41*41]; // TODO: hardcoded for now
};

bool descriptor_compute(const uint8_t * image, int width, int height, int stride, float cx, float cy, int side_length, float radius, descriptor & desc);
float descriptor_dist2(const descriptor & d1, const descriptor & d2);

#endif
