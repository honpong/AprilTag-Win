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
    fast_detector_9 fast;
    int track_threshold = 5;
    int detect_threshold = 15;
    float radius = 5.5f;
    float min_match = 0.2f*0.2f;
    float good_match = 0.65f*0.65f;
    
    void init()
    {
        fast.init(width, height, stride, half_patch_width * 2 + 1, half_patch_width);
    }
    
    xy track(const uint8_t * im1, const uint8_t * im2, float predx, float predy, float radius, float min_score)
    {
        return fast.track(im1, im2, half_patch_width, half_patch_width, predx, predy, radius, track_threshold, min_score);
    }
    
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int winx, int winy, int winwidth, int winheight)
    {
        return fast.detect(im, mask, number_wanted, detect_threshold, winx, winy, winwidth, winheight);
    }

    bool is_trackable(int x, int y)
    {
        return (x > half_patch_width &&
                y > half_patch_width &&
                x < width-1-half_patch_width &&
                y < height-1-half_patch_width);
    }

    void add_track(const unsigned char *im, int x, int y, unsigned char *patch)
    {
        int full_patch = 2 * half_patch_width + 1;
        for(int py = 0; py < full_patch; ++py) {
            for(int px = 0; px <= full_patch; ++px) {
                patch[py * full_patch + px] = im[x + px - half_patch_width + (y + py - half_patch_width) * width];
            }
        }
    }
};

#endif
