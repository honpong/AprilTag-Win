#ifndef __STEREO_H
#define __STEREO_H

#include "model.h"

extern "C" {
#include "cor.h"
}

typedef struct _stereo_state {
  int width;
  int height;
  int frame_number;
  uint8_t * frame;

  state_vector T;
  state_vector W;
  state_vector Tc;
  state_vector Wc;
  state_scalar focal_length;
  state_scalar center_x;
  state_scalar center_y;
  state_scalar k1, k2, k3;

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
bool stereo_should_save_state(struct filter * f, stereo_state s);
stereo_state stereo_save_state(struct filter * f, uint8_t * frame);
void stereo_free_state(stereo_state s);

// Returns a fundamental matrix between s2 and s1
m4 stereo_preprocess(stereo_state * s1, stereo_state * s2);
// Triangulates a feature in s2 by finding a correspondence in s1 and using the motion estimate to
// establish the baseline
v4 stereo_triangulate(stereo_state * s1, stereo_state * s2, m4 F, int s2_x1, int s2_y1);

m4 eight_point_F(v4 p1[], v4 p2[], int npts);

#endif
