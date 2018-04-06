#pragma once

#include "fast_tracker.h"

fast_tracker::xy *platform_fast_detect(const tracker::image &image, scaled_mask &mask, size_t need, size_t &found);
