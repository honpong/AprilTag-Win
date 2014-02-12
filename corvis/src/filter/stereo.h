#ifndef __STEREO_H
#define __STEREO_H

#include "model.h"

extern "C" {
#include "cor.h"
}

enum stereo_status_code {
    stereo_status_success = 1,
    stereo_status_error_triangulate,
    stereo_status_error_correspondence,
    stereo_status_error_too_few_points,
};

typedef struct _stereo_state {
  int width;
  int height;
  int frame_number;
  uint8_t * frame;

  v4 T;
  v4 W;
  v4 Tc;
  v4 Wc;
  f_t focal_length;
  f_t center_x;
  f_t center_y;
  f_t k1, k2, k3;

  list<state_vision_feature> features;
} stereo_state;

/* This module is used by:
 * - saving a state once features are acquired and stereo is enabled
 * - using stereo_should_save_state to check if the saved state should be refreshed, stereo_save_state to generate a new saved state, and stereo_free_state to free a saved state which is no longer required
 * - when the filter stops, stereo_save_state is called again with the last seen frame
 * - stereo_preprocess returns a fundamental matrix between the current frame (s2) and a previous frame (s1)
 * - stereo_triangulate is repeatedly called when a user clicks on a new point to measure
 */

/* 
 * stereo_should_save_state returns true when the intersection between the features the filter
 * knows about currently and the saved previous state (s) is less than 15 features
 */
bool stereo_should_save_state(struct filter * f, const stereo_state & s);
stereo_state stereo_save_state(struct filter * f, uint8_t * frame);
void stereo_free_state(stereo_state s);

// Computes a fundamental matrix between s2 and s1 and stores it in F. Can return stereo_status_success or stereo_status_error_too_few_points
enum stereo_status_code stereo_preprocess(const stereo_state & s1, const stereo_state & s2, m4 & F);
// Triangulates a feature in s2 by finding a correspondence in s1 and using the motion estimate to
// establish the baseline. Can return:
// stereo_status_success
// stereo_status_error_negative_depth
// stereo_status_error_too_few_points
enum stereo_status_code stereo_triangulate(const stereo_state & s1, const stereo_state & s2, m4 F, int s2_x1, int s2_y1, v4 & intersection);

m4 eight_point_F(v4 p1[], v4 p2[], int npts);

#endif
