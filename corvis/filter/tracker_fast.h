#ifndef __TRACKER_FAST_H
#define __TRACKER_FAST_H

extern "C" {
#include "cor.h"
}

#include "tracker.h"

feature_t track_fast(uint8_t * im1, uint8_t * im2, int width, int height, int currentx, int currenty, int x1, int y1, int x2, int y2, float & error);

#endif
