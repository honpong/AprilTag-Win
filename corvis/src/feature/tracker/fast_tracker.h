#ifndef __FAST_TRACKER_H__
#define __FAST_TRACKER_H__

#include "tracker.h"
#include "fast_constants.h"
#include "fast.h"

class fast_tracker : public tracker
{
    bool is_trackable(int x, int y, int width, int height)
    {
        return (x > half_patch_width &&
                y > half_patch_width &&
                x < width-1-half_patch_width &&
                y < height-1-half_patch_width);
    }

    template<typename Descriptor>
    struct fast_feature: public tracker::feature
    {
        fast_feature(float x_, float y_, const tracker::image& image) : x(x_), y(y_)
        {
            descriptor.compute_descriptor(x, y, image);
        }
        float x, y;
        Descriptor descriptor;
    };


private:
    fast_detector_9 fast;

public:
    fast_tracker() {}
    virtual ~fast_tracker() {}
    virtual std::vector<feature_track> &detect(const image &image, const std::vector<feature_track *> &current, size_t number_desired) override;
    virtual void track(const image &image, std::vector<feature_track *> &tracks) override;
};

#endif
