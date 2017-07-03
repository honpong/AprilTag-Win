/*
 * shave_tracker.h
 *
 *  Created on: Nov 24, 2016
 *      Author: administrator
 */

#ifndef SHAVE_TRACKER_H_
#define SHAVE_TRACKER_H_

#include "fast_tracker.h"

constexpr int fast_detect_controller_max_features = 400;
constexpr int fast_detect_controller_min_features = 200;
constexpr int fast_detect_controller_min_threshold = 15;
constexpr int fast_detect_controller_max_threshold = 70;
constexpr int fast_detect_controller_threshold_up_step = 5;
constexpr int fast_detect_controller_threshold_down_step = 10;
constexpr int fast_detect_controller_high_hist = 1;
constexpr int fast_detect_controller_low_hist = 2;

#include "filter.h"
#include "commonDefs.hpp"
#include "stereo_commonDefs.hpp"
#include <OsDrvSvu.h>
#include <OsDrvShaveL2Cache.h>
#include "adaptive_controller.h"

class shave_tracker : public fast_tracker
{

private:
    uint64_t next_id = 0;
    void detectShave(const image &image, const std::vector<point> &features, int number_desired);
    void detect2Shave(const image &image, const std::vector<point> &features, int number_desired);
    void detectMultipleShave(const image &image, int number_desired);
    void sortFeatures(const image &image, int number_desired);
    void trackShave(std::vector<TrackingData>& trackingData, const image& image);
    void trackMultipleShave(std::vector<TrackingData>& trackingData, const image& image);
    void prepTrackingData(std::vector<TrackingData>& trackingData, std::vector<prediction> &predictions);
    void processTrackingResult(std::vector<prediction>& predictions);
    void new_keypoint_other_keypoint_mearge(const std::vector<tracker::point> & kp1, std::vector<tracker::point> & new_keypoints);

    int shavesToUse;
    static osDrvSvuHandler_t s_handler[12];
    static bool s_shavesOpened;
    AdaptiveController  m_thresholdController;
    // XXX to remove, not in use
    double m_featuresDetectionTime;
    double m_featuresSortingTime;
    double m_featuresCollectionTime;
    int m_lastDetectedFeatures;

public:
    shave_tracker();
    ~shave_tracker();
    virtual std::vector<point> &detect(const image &image, const std::vector<point> &features, int number_desired) override;
    virtual std::vector<prediction> &track(const image &image, std::vector<prediction> &predictions) override;
    virtual void drop_feature(uint64_t feature_id) override;
    void stereo_matching_full_shave (const std::vector<tracker::point> & kp1, std::vector<tracker::point> & kp2,const fast_tracker::feature * f1_group[] ,const fast_tracker::feature * f2_group[], state_camera & camera1, state_camera & camera2, std::vector<tracker::point> * new_keypoints_p);

};

#endif /* SHAVE_TRACKER_H_ */
