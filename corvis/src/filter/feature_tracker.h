#ifndef __FEATURE_TRACKER_H__
#define __FEATURE_TRACKER_H__

#include <vector>

typedef struct {
    // timestamps are in microseconds
    int64_t time_us;
    // measurement is in radians / s^2
    float w[3];
} gyro_measurement;

typedef struct {
    uint64_t id;
    float x, y;
    // scores are > 0, higher scores are better detections / tracks
    float score;
} tracker_point;

typedef struct {
    int64_t time_us;
    // images are luminance only, stride = width
    uint8_t * image;
    int width_px;
    int height_px;
} tracker_image;

/**
 orientation Orientation of camera relative to the gyro
    Flat array of 9 floats, corresponding to 3x3 rotation matrix in row-major order:
    [R00 R01 R02]
    [R10 R11 R12]
    [R20 R21 R22]
 image_width_px Image width in pixels
 image_height_px Image height in pixels
 center_x_px Horizontal principal point of camera in pixels
 center_y_px Horizontal principal point of camera in pixels
 focal_length_px Focal length of camera in pixels
 K camera radial distortion parameters with:
    // r2 is the squared radius from the center of the image in
    // normalized coordinates
    kr = 1. + r2 * (k1 + r2 * (k2 + r2 * k3));
 */

typedef struct {
    float orientation[9];
    int image_width_px;
    int image_height_px;
    float center_x_px;
    float center_y_px;
    float focal_length_x_px;
    float focal_length_y_px;
    float K[3];
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
     @param previous_image The previous image where the feature was
     @param current_image The image to track in
     @param gyro_measurements A vector of gyro measurements seen between the two images
     @param features The points in the previous image
     @param predictions The predicted location in the current image

     Returns a vector of tracker_point tracks. Dropped features are not included. Tracks with high scores should be considered more accurate
     */
    virtual std::vector<tracker_point> track(const tracker_image & previous_image,
            const tracker_image & current_image,
            const std::vector<gyro_measurement> & gyro_measurements,
            const std::vector<tracker_point> & features,
            const std::vector<std::vector<tracker_point> > & predictions) = 0;

    /*
     @param feature_ids A vector of feature ids which are no longer tracked. Free any internal storage related to them.
     */
    virtual void drop_features(const std::vector<uint64_t> feature_ids) = 0;
};

#endif
