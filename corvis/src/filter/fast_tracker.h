#ifndef __FAST_TRACKER_H__
#define __FAST_TRACKER_H__

#include "feature_tracker.h"

#include "fast_detector/fast.h"

#include <map>

class fast_tracker : public tracker
{
    static constexpr int half_patch_width = 3;
    static constexpr int full_patch_width = half_patch_width * 2 + 1;

    bool is_trackable(int x, int y, int width, int height)
    {
        return (x > half_patch_width &&
                y > half_patch_width &&
                x < width-1-half_patch_width &&
                y < height-1-half_patch_width);
    }

    struct feature
    {
        feature(int _x, int _y, const uint8_t * im, int width) : x(_x), y(_y)
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


private:
    std::map<uint64_t, feature> features;
    fast_detector_9 fast;
    uint64_t next_id = 0;

public:
    fast_tracker() {};
    fast_tracker(const intrinsics params) {};
    std::vector<point> detect(const image &image, int number_desired);
    std::vector<point> track(const image &image,
                             const std::vector<point> &features,
                             const std::vector<std::vector<point> > &predictions);
    void drop_features(const std::vector<uint64_t>cfeature_ids);
};

#endif
