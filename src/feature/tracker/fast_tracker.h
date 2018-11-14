#ifndef __FAST_TRACKER_H__
#define __FAST_TRACKER_H__

#include "tracker.h"
#include "fast_constants.h"

class fast_tracker : public tracker
{
public:
    template<int border_size>
    static bool is_trackable(int x, int y, int width, int height)
    {
        return 0 <= (x-border_size) && (x+border_size) < width
            && 0 <= (y-border_size) && (y+border_size) < height;
    }

public:
    template<typename Descriptor>
    struct fast_feature: public tracker::feature
    {
        fast_feature(float x_, float y_, const tracker::image& image) : descriptor(x_, y_, image) {}
        fast_feature(uint64_t id, float x_, float y_, const tracker::image& image) : feature(id),
              descriptor(x_, y_, image) {}
        fast_feature(uint64_t id, const Descriptor &descriptor_) : feature(id),
              descriptor(descriptor_) {}
        fast_feature(uint64_t id) : feature(id) {}
        Descriptor descriptor;
    };


    typedef struct { float x, y, score, reserved; } xy;

    static bool xy_comp(const xy &a, const xy &b) { return a.score != b.score ? a.score > b.score : a.y != b.y ? a.y < b.y : a.x < b.x; }
private:
    int pixel[16];
    int xsize, ysize, stride, patch_stride;
    void init(const int x, const int y, const int s, const int ps);
    std::vector<xy> features;

public:
    fast_tracker() : xsize(0), ysize(0), stride(0), patch_stride(0) {}
    virtual ~fast_tracker() {}
    virtual std::vector<feature_track> &detect(const image &image, const std::vector<feature_position> &current, size_t number_desired) override;
    virtual void track(const image &image, std::vector<feature_track *> &tracks) override;

protected:
    void init_mask(const image &image, const std::vector<feature_position> &current);
    std::vector<feature_track> &finalize_detect(xy *features_begin, xy *features_end, const image &image, size_t number_desired);
    std::vector<feature_track> &non_maximum_suppression(xy *features_begin, xy *features_end, const image& image, size_t number_desired);
};

#endif
