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
#include "Shave.h"

class shave_tracker : public fast_tracker
{

private:
    uint64_t next_id = 0;
    void trackMultipleShave(std::vector<TrackingData>& trackingData, const image& image);
    void prepTrackingData(std::vector<TrackingData>& trackingData, std::vector<tracker::feature_track *> &predictions);
    void processTrackingResult(std::vector<tracker::feature_track *> &predictions);

public:
    virtual std::vector<tracker::feature_track> &detect(const image &image, const std::vector<tracker::feature_position> &features, size_t number_desired) override;
    virtual void track(const image &image, std::vector<tracker::feature_track *> &predictions) override;
    void stereo_matching_full_shave(struct filter *f, rc_Sensor camera1_id, rc_Sensor camera2_id);

};

#endif /* SHAVE_TRACKER_H_ */
