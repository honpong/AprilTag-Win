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
#include "image_defines.h"
#include "stereo_commonDefs.hpp"
#include "adaptive_controller.h"
#include "Shave.h"

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

    AdaptiveController  m_thresholdController;
    int m_lastDetectedFeatures;

public:
    shave_tracker();

    virtual std::vector<tracker::feature_track> &detect(const image &image, const std::vector<tracker::feature_position> &features, size_t number_desired) override;
    virtual void track(const image &image, std::vector<tracker::feature_track *> &predictions) override;
    void stereo_matching_full_shave(struct filter *f, rc_Sensor camera1_id, rc_Sensor camera2_id);

};

#endif /* SHAVE_TRACKER_H_ */
