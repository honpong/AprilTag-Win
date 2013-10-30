#include "tracker_fast.h"
#include "fast_detector/fast.h"

feature_t track_fast(uint8_t * im1, uint8_t * im2, int width, int height, int currentx, int currenty, int x1, int y1, int x2, int y2, float & error)
{
    static fast_detector_9 fast;
    static bool init = false;
    int bthresh = 10;

    if(!init) {
        fast.init(width, height, width);
        init = true;
    }

    feature_t feature;
    xy pt = fast.track(im1, im2, currentx, currenty, x1, y1, x2, y2, bthresh);
    feature.x = pt.x;
    feature.y = pt.y;
    error = pt.score;
    return feature;
}

