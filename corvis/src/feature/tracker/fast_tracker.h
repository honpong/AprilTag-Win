#ifndef __FAST_TRACKER_H__
#define __FAST_TRACKER_H__

#include "tracker.h"
#include "fast_constants.h"
#include "fast.h"

class fast_tracker : public tracker
{
    template<int border_size>
    bool is_trackable(int x, int y, int width, int height)
    {
        return (x > border_size &&
                y > border_size &&
                x < width-1-border_size &&
                y < height-1-border_size);
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
