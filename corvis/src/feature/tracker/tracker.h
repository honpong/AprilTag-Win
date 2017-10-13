#ifndef __FEATURE_TRACKER_H__
#define __FEATURE_TRACKER_H__

#include <vector>
#include <cstdint>
#include <memory>
#include <atomic>
#include <cmath>

#include "scaled_mask.h"

struct tracker {
    struct feature {
        uint64_t id;
        static std::atomic_uint_fast64_t next_id;
        feature(): id(next_id++) {}
        feature(uint64_t id_): id(id_) {}
        virtual ~feature() {}
    };

    struct group_track {
        uint64_t group_id;
        float x,y;
    };

    struct feature_track{
        std::shared_ptr<struct feature> feature;
        float x, y;
        float dx = 0, dy = 0;
        float pred_x = INFINITY, pred_y = INFINITY;
        float score; // scores are > 0, higher scores are better detections / tracks
        float depth = 0;
        float error = 0;
        std::vector<group_track> group_tracks; // keeps group id/measurement
        bool found() const { return x != INFINITY; }
        feature_track(std::shared_ptr<struct feature> feature_, float x_, float y_, float score_)
            : feature(feature_), x(x_), y(y_), score(score_) {}
    };

    typedef struct {
        const uint8_t *image;
        int width_px;
        int height_px;
        int stride_px;
    } image;

    std::unique_ptr<scaled_mask> mask;

    std::vector<feature_track> feature_points;
    std::vector<feature_track *> tracks; // reusable storage passed to track() and detect()
    /*
     @param image  The image to use for feature detection
     @param number_desired  The desired number of features, function can return less or more
     @param features  Currently tracked features

     Returns a reference to a vector (using feature_points above for storage) of newly detected features with higher scored points being preferred
     */
    virtual std::vector<feature_track> &detect(const image &image, const std::vector<feature_track *> &current_features, size_t number_desired) = 0;

    /*
     @param current_image The image to track in
     @param tracks Includes the current and predicted locations in the current image

     Updates the vector of tracks, with found and score set
     */
    virtual void track(const image &image, std::vector<feature_track *> &tracks) = 0;

    virtual ~tracker() {}
};

#endif
