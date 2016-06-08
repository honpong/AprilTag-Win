#ifndef __TRACKER_PYRAMID_H
#define __TRACKER_PYRAMID_H

extern "C" {
#include "cor.h"
}

void tracker_pyramid_init(int width, int height, int stride);
feature_t tracker_pyramid_track(uint8_t * im1, uint8_t * im2, int currentx, int currenty, int x1, int y1, int x2, int y2);

#endif
