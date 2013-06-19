#include "tracker_fast.h"
#include "fast_detector/fast.h"

fast_detector * fast = 0;
int bthresh = 20;

void tracker_fast_init(int width, int height, int stride)
{
    fast = new fast_detector(width, height, stride);
}

feature_t tracker_fast_track(uint8_t * im1, uint8_t * im2, int currentx, int currenty, int x1, int y1, int x2, int y2)
{
    feature_t feature;
    xy pt = fast->track(im1, im2, currentx, currenty, x1, y1, x2, y2, bthresh);
    feature.x = pt.x;
    feature.y = pt.y;
    return feature;
}

