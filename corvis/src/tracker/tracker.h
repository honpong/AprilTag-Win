#ifndef __FEATURE_TRACKER_H__
#define __FEATURE_TRACKER_H__

#include <vector>
#include <cstdint>
#include <memory>
#include <atomic>

#include "scaled_mask.h"

struct tracker {
    struct feature {
        uint64_t id;
        static std::atomic_uint_fast64_t next_id;
        feature(): id(next_id++) {}
        virtual ~feature() {}
    };
    
    struct point {
        std::shared_ptr<struct feature> feature;
        float x, y;
        float score; // scores are > 0, higher scores are better detections / tracks
        point(std::shared_ptr<struct feature> feature_, float x_, float y_, float score_) : feature(feature_), x(x_), y(y_), score(score_) {}
    };

    struct feature_track : public point {
        bool found;
        float pred_x, pred_y;
        feature_track(std::shared_ptr<struct feature> feature_, float x_, float y_, float pred_x_, float pred_y_, float score_)
            : point(feature_, x_, y_, score_), found(false), pred_x(pred_x_), pred_y(pred_y_) {}
    };

    typedef struct {
        const uint8_t *image;
        int width_px;
        int height_px;
        int stride_px;
    } image;

    std::unique_ptr<scaled_mask> mask;

    std::vector<feature_track> feature_points;
    std::vector<point> current_features; // reuasable storage passed to detect()
    std::vector<feature_track *> tracks; // reusable storage passed to and returned from track()
    /*
     @param image  The image to use for feature detection
     @param number_desired  The desired number of features, function can return less or more
     @param features  Currently tracked features

     Returns a reference to a vector (using feature_points above for storage) of newly detected features with higher scored points being preferred
     */
    virtual std::vector<feature_track> &detect(const image &image, const std::vector<point> &current_features, int number_desired) = 0;

    /*
     @param current_image The image to track in
     @param tracks Includes the current and predicted locations in the current image

     Updates the vector of tracks, with found and score set
     */
    virtual void track(const image &image, std::vector<feature_track *> &tracks) = 0;

    virtual ~tracker() {}
};

#endif
