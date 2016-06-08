#ifndef __FAST_TRACKER_H__
#define __FAST_TRACKER_H__

#include "feature_tracker.h"

#include "fast_detector/fast.h"

#include <map>

const static int half_patch_width = 3;
const static int full_patch_width = half_patch_width * 2 + 1;

class FastFeature
{
public:
    FastFeature(int _x, int _y, const uint8_t * im, int width) : x(_x), y(_y)
    {
        for(int py = 0; py < full_patch_width; ++py) {
            for(int px = 0; px <= full_patch_width; ++px) {
                patch[py * full_patch_width + px] = im[(int)x + px - half_patch_width + ((int)y + py - half_patch_width) * width];
            }
        }
    }
    uint8_t patch[full_patch_width*full_patch_width];
    float x, y;
};


class FastTracker : FeatureTracker
{
private:
    std::map<uint64_t, FastFeature> features;
    fast_detector_9 fast;
    uint64_t next_id = 0;

public:
    FastTracker() {};
    FastTracker(const camera_parameters params) {};
    std::vector<tracker_point> detect(const tracker_image & image, int number_desired);
    std::vector<tracker_point> track(const tracker_image & current_image,
                                     const std::vector<tracker_point> & features,
                                     const std::vector<std::vector<tracker_point> > & predictions);
    void drop_features(const std::vector<uint64_t>cfeature_ids);
};

#endif
