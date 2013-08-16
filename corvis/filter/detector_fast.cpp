#include "detector_fast.h"
#include "fast_detector/fast.h"

void detect_fast(const uint8_t * im, const uint8_t * mask, int width, int height, vector<feature_t> & keypoints, int number_wanted, int winx, int winy, int winwidth, int winheight)
{
    int bthresh = 40;

    fast_detector_9 detector(width, height, width);
    vector<xy> &features = detector.detect(im, mask, number_wanted, bthresh, winx, winy, winwidth, winheight);

    keypoints.clear();
    for(int i = 0; i < features.size(); ++i) {
        feature_t t;
        t.x = features[i].x;
        t.y = features[i].y;
        keypoints.push_back(t);
    }
}
