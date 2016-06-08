#ifndef __FEATURE_TRACKER_H__
#define __FEATURE_TRACKER_H__

#include <vector>

typedef struct {
    uint64_t id;
    float x, y;
    // scores are > 0, higher scores are better detections / tracks
    float score;
} tracker_point;

typedef struct {
    // images are luminance only, stride = width
    uint8_t * image;
    int width_px;
    int height_px;
} tracker_image;

/**
 image_width_px Image width in pixels
 image_height_px Image height in pixels
 center_x_px Horizontal principal point of camera in pixels
 center_y_px Horizontal principal point of camera in pixels
 focal_length_px Focal length of camera in pixels
 */

typedef struct {
    int image_width_px;
    int image_height_px;
    float center_x_px;
    float center_y_px;
    float focal_length_x_px;
    float focal_length_y_px;
} camera_parameters;

class FeatureTracker {
public:
    // Derived class constructors can add extra parameters if needed
    // FeatureTracker(const camera_parameters camera_param) {};

    /*
     @param image The image to use for feature detection
     @param number_desired The desired number of features, function can return less or more

     Returns a vector of tracker_point detections, with higher scored points being preferred
     */
    virtual std::vector<tracker_point> detect(const tracker_image & image, int number_desired) = 0;

    /*
     @param current_image The image to track in
     @param features The points in the previous image
     @param predictions The predicted location in the current image

     Returns a vector of tracker_point tracks. Dropped features are not included. Tracks with high scores should be considered more accurate
     */
    virtual std::vector<tracker_point> track(const tracker_image & current_image,
                                             const std::vector<tracker_point> & features,
                                             const std::vector<std::vector<tracker_point> > & predictions) = 0;

    /*
     @param feature_ids A vector of feature ids which are no longer tracked. Free any internal storage related to them.
     */
    virtual void drop_features(const std::vector<uint64_t> feature_ids) = 0;
};

#endif
