//
//  feature_tracker.cpp
//  corvis
//
//  Created by Eagle Jones on 5/19/17.
//  Copyright 2017 Intel
//

#include "tracker.h"

std::atomic_uint_fast64_t tracker::feature::next_id {0};
