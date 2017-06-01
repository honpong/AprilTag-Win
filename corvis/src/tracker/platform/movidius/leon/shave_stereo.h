/*
 * shave_stereo.h
 *
 *  Created on: May 16, 2017
 *      Author: Amir shalev
 */

#ifndef SHAVE_STEREO_H_
#define SHAVE_STEREO_H_


#include "filter.h"
#include "fast_tracker.h"
#include <OsDrvSvu.h>
#include "stereo_commonDefs.hpp"

class shave_stereo
{

private:
    int64_t next_id = 0;
    static osDrvSvuHandler_t s_handler[12];
    static bool s_shavesOpened_st;

    void keypoint_intersect_shave ( const std::vector<tracker::point> & kp1 , std::vector<tracker::point> & kp2 ,state_camera & camera1, state_camera & camera2 );
    void keypoint_compare_leon   ( const std::vector<tracker::point> & kp1 , std::vector<tracker::point> & kp2 ,const fast_tracker::feature * f1_group[] ,const fast_tracker::feature * f2_group[] , std::vector<tracker::point> &  new_keypoints ) ;

public:

    shave_stereo();
    ~shave_stereo();
    void stereo_matching_shave (const std::vector<tracker::point> & kp1 , std::vector<tracker::point> & kp2 ,const fast_tracker::feature * f1_group[] ,const fast_tracker::feature * f2_group[] , state_camera & camera1, state_camera & camera2 , std::vector<tracker::point> * new_keypoints_p  );

};

#endif /* SHAVE_STEREO_H_ */
