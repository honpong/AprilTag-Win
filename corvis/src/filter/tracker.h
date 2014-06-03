#ifndef __TRACKER_H
#define __TRACKER_H

#include "fast_detector/fast.h"
#include "scaled_mask.h"

extern "C" {
#include "cor.h"
}

struct tracker {
    int width;
    int height;
    int stride;
    uint8_t *im1, *im2;
    struct mapbuffer *sink;
    fast_detector_9 fast;
    
    void init()
    {
        fast.init(width, height, stride);
    }
    
    feature_t track(uint8_t * im1, uint8_t * im2, int currentx, int currenty, int x1, int y1, int x2, int y2, float & error)
    {
        int bthresh = 10;
        xy pt = fast.track(im1, im2, currentx, currenty, x1, y1, x2, y2, bthresh);
        feature_t feature;
        feature.x = pt.x;
        feature.y = pt.y;
        error = pt.score;
        return feature;
    }
    
    vector<xy> &detect(const unsigned char *im, const scaled_mask *mask, int number_wanted, int winx, int winy, int winwidth, int winheight)
    {
        int bthresh = 40;
        return fast.detect(im, mask, number_wanted, bthresh, winx, winy, winwidth, winheight);
    }
};

#endif
