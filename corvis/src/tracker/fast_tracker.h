#ifndef __FAST_TRACKER_H__
#define __FAST_TRACKER_H__

#include "tracker.h"
#include "fast_constants.h"
#include "fast_detector/fast.h"

#include <map>

class fast_tracker : public tracker
{
public:
    bool is_trackable(int x, int y, int width, int height)
    {
        return (x > half_patch_width &&
                y > half_patch_width &&
                x < width-1-half_patch_width &&
                y < height-1-half_patch_width);
    }

    struct feature
    {
        feature(int x, int y, const uint8_t * im, int stride) : dx(0), dy(0)
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

protected:
    std::map<uint64_t, feature> feature_map;

private:
    fast_detector_9 fast;
    uint64_t next_id = 0;

public:
    fast_tracker() {}
    virtual ~fast_tracker() {}
    virtual std::vector<point> &detect(const image &image, const std::vector<point> &features, int number_desired) override;
    virtual std::vector<prediction> &track(const image &image, std::vector<prediction> &predictions) override;
    virtual void drop_feature(uint64_t feature_id) override;
    virtual void drop_all_features() override;
    const struct feature & get_feature(uint64_t feature_id) const {
        return feature_map.at(feature_id);
    }
};

#endif
