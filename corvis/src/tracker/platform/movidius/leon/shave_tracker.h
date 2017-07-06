/*
 * shave_tracker.h
 *
 *  Created on: Nov 24, 2016
 *      Author: administrator
 */

#ifndef SHAVE_TRACKER_H_
#define SHAVE_TRACKER_H_

#include "fast_tracker.h"
#include "filter.h"
#include "commonDefs.hpp"
#include "stereo_commonDefs.hpp"
#include "adaptive_controller.h"
#include "Shave.h"

#define TRACKER_SHAVES_USED  4

constexpr int fast_detect_controller_max_features = 400;
constexpr int fast_detect_controller_min_features = 200;
constexpr int fast_detect_controller_min_threshold = 15;
constexpr int fast_detect_controller_max_threshold = 70;
constexpr int fast_detect_controller_threshold_up_step = 5;
constexpr int fast_detect_controller_threshold_down_step = 10;
constexpr int fast_detect_controller_high_hist = 1;
constexpr int fast_detect_controller_low_hist = 2;



class shave_tracker : public fast_tracker
{

private:
    uint64_t next_id = 0;
    void detectMultipleShave(const image &image);
    void sortFeatures(const image &image, int number_desired);
    void trackMultipleShave(std::vector<TrackingData>& trackingData, const image& image);
    void prepTrackingData(std::vector<TrackingData>& trackingData, std::vector<prediction> &predictions);
    void processTrackingResult(std::vector<prediction>& predictions);
    void new_keypoint_other_keypoint_mearge(const std::vector<tracker::point> & kp1, std::vector<tracker::point> & new_keypoints);

    int shavesToUse;
    AdaptiveController  m_thresholdController;
    int m_lastDetectedFeatures;
    Shave* shaves[TRACKER_SHAVES_USED];

public:
    shave_tracker();

    virtual std::vector<point> &detect(const image &image, const std::vector<point> &features, int number_desired) override;
    virtual std::vector<prediction> &track(const image &image, std::vector<prediction> &predictions) override;
    virtual void drop_feature(uint64_t feature_id) override;
    void stereo_matching_full_shave (const std::vector<tracker::point> & kp1, std::vector<tracker::point> & kp2,const fast_tracker::feature * f1_group[] ,const fast_tracker::feature * f2_group[], state_camera & camera1, state_camera & camera2, std::vector<tracker::point> * new_keypoints_p);

};

#endif /* SHAVE_TRACKER_H_ */
