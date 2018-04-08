#pragma once

#include "vec4.h"
#include "fast_tracker.h"
#include "patch_orb_descriptor.h"
#include "orb_constants.h"
#include "Shave.h"

void compute_orb_multiple_shaves(const tracker::image &image, fast_tracker::fast_feature<patch_orb_descriptor>* keypoints[], const v2* keypoints_xy[], size_t num_kpoints, int& actual_num_descriptors);
