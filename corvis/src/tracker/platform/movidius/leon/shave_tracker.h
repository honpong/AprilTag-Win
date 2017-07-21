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
    void prepTrackingData(std::vector<TrackingData>& trackingData, std::vector<tracker::feature_track *> &predictions);
    void processTrackingResult(std::vector<tracker::feature_track *> &predictions);

    int shavesToUse;
    AdaptiveController  m_thresholdController;
    int m_lastDetectedFeatures;
    Shave* shaves[TRACKER_SHAVES_USED];

public:
    shave_tracker();

    virtual std::vector<tracker::feature_track> &detect(const image &image, const std::vector<tracker::feature_track *> &features, size_t number_desired) override;
    virtual void track(const image &image, std::vector<tracker::feature_track *> &predictions) override;
    void stereo_matching_full_shave(tracker::feature_track * f1_group[], size_t n1, const tracker::feature_track * f2_group[], size_t n2, state_camera & camera1, state_camera & camera2);

};

#endif /* SHAVE_TRACKER_H_ */
