#include "detector_fast.h"
#include "fast_detector/fast.h"

void detect_fast(const uint8_t * im, const uint8_t * mask, int width, int height, vector<feature_t> & keypoints, int number_wanted, int winx, int winy, int winwidth, int winheight)
{
    static fast_detector_9 fast;
    static bool init = false;
    int bthresh = 40;

    if(!init) {
        fast.init(width, height, width);
        init = true;
    }
    vector<xy> &features = fast.detect(im, mask, number_wanted, bthresh, winx, winy, winwidth, winheight);

    keypoints.clear();
    for(int i = 0; i < features.size(); ++i) {
        feature_t t;
        t.x = features[i].x;
        t.y = features[i].y;
        keypoints.push_back(t);
    }
}
