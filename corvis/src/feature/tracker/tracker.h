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
        feature(feature &&) = default;
        feature & operator=(feature &&) = default;
        feature(const feature &) = delete;
        feature & operator=(const feature &) = delete;
        virtual ~feature() {}
    };

    struct feature_position {
        std::shared_ptr<struct feature> feature;
        float x, y;
        feature_position(std::shared_ptr<struct feature> &&feature_, float x_, float y_)
            : feature(std::move(feature_)), x(x_), y(y_) {}
        feature_position(std::shared_ptr<struct feature> &feature_, float x_, float y_)
            : feature(feature_), x(x_), y(y_) {}
    };

    struct feature_track : public feature_position {
        float dx = 0, dy = 0;
        float pred_x = INFINITY, pred_y = INFINITY;
        float score; // scores are > 0, higher scores are better detections / tracks
        bool found() const { return x != INFINITY; }
        feature_track(std::shared_ptr<struct feature> &&feature_, float x_, float y_, float score_)
            : feature_position(std::move(feature_), x_, y_), score(score_) {}
        feature_track(std::shared_ptr<struct feature> &feature_, float x_, float y_, float score_)
            : feature_position(feature_, x_, y_), score(score_) {}

        feature_track(feature_track &&) = default;
        feature_track & operator=(feature_track &&) = default;
        feature_track(const feature_track &) = delete;
        feature_track & operator=(const feature_track &) = delete;
    };

    typedef struct {
        const uint8_t *image;
        int width_px;
        int height_px;
        int stride_px;
    } image;

    std::unique_ptr<scaled_mask> mask;

    std::vector<feature_track> feature_points;
    std::vector<tracker::feature_position> avoid; // reusable storage passed to detect()
    std::vector<feature_track *> tracks; // reusable storage passed to track()
    /*
     @param image  The image to use for feature detection
     @param number_desired  The desired number of features, function can return less or more
     @param features  Currently tracked features

     Returns a reference to a vector (using feature_points above for storage) of newly detected features with higher scored points being preferred
     */
    virtual std::vector<feature_track> &detect(const image &image, const std::vector<feature_position> &current_features, size_t number_desired) = 0;

    /*
     @param current_image The image to track in
     @param tracks Includes the current and predicted locations in the current image

     Updates the vector of tracks, with found and score set
     */
    virtual void track(const image &image, std::vector<feature_track *> &tracks) = 0;

    virtual ~tracker() {}
};

#endif
