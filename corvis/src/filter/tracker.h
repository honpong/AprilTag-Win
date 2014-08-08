#ifndef __TRACKER_H
#define __TRACKER_H

#include "fast_detector/fast.h"
#include "scaled_mask.h"

extern "C" {
#include "../cor/cor_types.h"
}

struct tracker {
    int width;
    int height;
    int stride;
    const static int half_patch_width = 3;
    uint8_t *im1, *im2;
    struct mapbuffer *sink;
    fast_detector_9 fast;
    
    void init()
    {
        fast.init(width, height, stride, half_patch_width * 2 + 1, half_patch_width);
    }
    
    xy track(uint8_t * im1, uint8_t * im2, float predx, float predy, float radius, float min_score)
    {
        int bthresh = 10;
        return fast.track(im1, im2, half_patch_width, half_patch_width, predx, predy, radius, bthresh, min_score);
    }
    
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int winx, int winy, int winwidth, int winheight)
    {
        int bthresh = 40;
        return fast.detect(im, mask, number_wanted, bthresh, winx, winy, winwidth, winheight);
    }
};

#endif
