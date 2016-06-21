#ifndef __FEATURE_TRACKER_H__
#define __FEATURE_TRACKER_H__

#include <vector>
#include <cstdint>

#include "scaled_mask.h"

struct tracker {
    struct point {
        uint64_t id;
        float x, y;
        float score; // scores are > 0, higher scores are better detections / tracks
        point(uint64_t id_, float x_, float y_, float score_) : id(id_), x(x_), y(y_), score(score_) {}
    };

    struct prediction : public point {
        bool found;
        float prev_x, prev_y;
        prediction(uint64_t id_, float prev_x_, float prev_y_, float pred_x_, float pred_y_)
            : point(id_, pred_x_, pred_y_, 0), found(false), prev_x(prev_x_), prev_y(prev_y_) {}
    };

    struct image {
        const uint8_t *image;
        int width_px;
        int height_px;
        int stride_px;
    };

    std::unique_ptr<scaled_mask> mask;

    std::vector<point> feature_points;
    /*
     @param image  The image to use for feature detection
     @param number_desired  The desired number of features, function can return less or more
     @param features  Currently tracked features

     Returns a reference to a vector (using feature_points above for storage) of newly detected features with higher scored points being preferred
     */
    virtual std::vector<point> &detect(const image &image, const std::vector<point> &features, int number_desired) = 0;

    /*
     @param current_image The image to track in
     @param predictions The current and predicted locations in the current image

     Returns the same vector of predictions as given but with found and score set
     */
    virtual std::vector<prediction> &track(const image &image, std::vector<prediction> &predictions) = 0;

    /*
     @param feature_ids A vector of feature ids which are no longer tracked. Free any internal storage related to them.
     */
    virtual void drop_feature(uint64_t feature_id) = 0;

    virtual ~tracker() {}
};

#endif
