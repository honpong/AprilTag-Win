#pragma once

#include "vec4.h"
#include "fast_tracker.h"
#include "patch_orb_descriptor.h"
#include "orb_constants.h"
#include "Shave.h"

typedef struct orb_desc_keypoint {
    fast_tracker::fast_feature<patch_orb_descriptor>* kp;
    v2 xy;
} orb_desc_keypoint;

void compute_orb_multiple_shaves(const tracker::image &image, orb_desc_keypoint *keypoints, size_t num_keypoints);
