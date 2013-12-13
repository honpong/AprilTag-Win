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

stereo_state stereo_save_state(struct filter * f, uint8_t * frame);
float stereo_measure(stereo_state * s1, stereo_state * s2, int s2_x1, int s2_y1, int s2_x2, int s2_y2);

// stereo_save_state(sturct stereo * s, struct filter * f, uint8_t * frame)

// need to save keyframe and state (minimally including a list of tracked
// features, R, and T)

#endif
