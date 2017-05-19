#ifndef __FAST_TRACKER_H__
#define __FAST_TRACKER_H__

#include "tracker.h"
#include "fast_constants.h"
#include "fast_detector/fast.h"

#include <map>

class fast_tracker : public tracker
{
    bool is_trackable(int x, int y, int width, int height)
    {
        return (x > half_patch_width &&
                y > half_patch_width &&
                x < width-1-half_patch_width &&
                y < height-1-half_patch_width);
    }

    struct fast_feature: public tracker::feature
    {
        fast_feature(int x, int y, const uint8_t * im, int stride) : dx(0), dy(0)
        {
            for(int py = 0; py < full_patch_width; ++py) {
                for(int px = 0; px < full_patch_width; ++px) {
                    patch[py * full_patch_width + px] = im[(int)x + px - half_patch_width + ((int)y + py - half_patch_width) * stride];
                }
            }
        }
        uint8_t patch[full_patch_width*full_patch_width];
        float dx, dy;
    };


private:
    fast_detector_9 fast;

public:
    fast_tracker() {}
    virtual ~fast_tracker() {}
    virtual std::vector<feature_track> &detect(const image &image, const std::vector<feature_track *> &current, int number_desired) override;
    virtual void track(const image &image, std::vector<feature_track *> &tracks) override;
};

#endif
