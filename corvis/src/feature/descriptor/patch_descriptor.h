/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
Pedro Pinies, Lina Paz
*********************************************************************************/
#pragma once
#include "tracker.h"
#include "fast_constants.h"
#include <array>

class patch_descriptor
{
public:
    static const int L = full_patch_width * full_patch_width; // descriptor length
    static const int full_patch_size = full_patch_width;
    static const int half_patch_size = half_patch_width;
    static const int min_score = fast_min_match;

    typedef std::array<unsigned char, L> TDescriptor;

    TDescriptor descriptor;

    static double distance(const TDescriptor &a, const TDescriptor &b);
    void compute_descriptor(float x, float y, const tracker::image& image);
};
