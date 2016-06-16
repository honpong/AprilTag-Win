#ifndef __FEATURE_TRACKER_H__
#define __FEATURE_TRACKER_H__

#include <vector>
#include <cstdint>

struct tracker {
    struct point {
        uint64_t id;
        float x, y;
        float score; // scores are > 0, higher scores are better detections / tracks
        point(uint64_t id_, float x_, float y_, float score_) : id(id_), x(x_), y(y_), score(score_) {}
    };

    struct image {
        const uint8_t *image;
        int width_px;
        int height_px;
        int stride_px;
    };

    /**
       image_width_px Image width in pixels
       image_height_px Image height in pixels
       center_x_px Horizontal principal point of camera in pixels
       center_y_px Horizontal principal point of camera in pixels
       focal_length_px Focal length of camera in pixels
    */
    typedef struct {
        int width_px;
        int height_px;
        float center_x_px;
        float center_y_px;
        float focal_length_x_px;
        float focal_length_y_px;
    } intrinsics;

    std::vector<point> feature_points;

    /*
     @param image The image to use for feature detection
     @param number_desired The desired number of features, function can return less or more

     Returns a vector of tracker_point detections, with higher scored points being preferred
     */
    virtual std::vector<point> &detect(const image &image, int number_desired) = 0;

    /*
     @param current_image The image to track in
     @param features The points in the previous image
     @param predictions The predicted location in the current image

     Returns a vector of tracker_point tracks. Dropped features are not included. Tracks with high scores should be considered more accurate
     */
    virtual std::vector<point> &track(const image &image, const std::vector<point> &predictions) = 0;

    /*
     @param feature_ids A vector of feature ids which are no longer tracked. Free any internal storage related to them.
     */
    virtual void drop_features(const std::vector<uint64_t> &feature_ids) = 0;
};

#endif
