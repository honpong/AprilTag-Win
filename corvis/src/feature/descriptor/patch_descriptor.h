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
#include "patch_constants.h"
#include <array>

class patch_descriptor
{
public:
    static const int L = full_patch_width * full_patch_width; // descriptor length
    static const int full_patch_size = full_patch_width;
    static const int half_patch_size = half_patch_width;
    static const int border_size = half_patch_size;
    static constexpr float bad_score = 0;
    static constexpr float min_score = patch_min_score;
    static constexpr float good_score = patch_good_score;

    std::array<unsigned char, L> descriptor;
    float mean{0}, variance{0};

    patch_descriptor(float x, float y, const tracker::image& image);
    patch_descriptor(const std::array<unsigned char, L> &d) : descriptor(d) {}
    static bool is_better(const float score1, const float score2) {return score1 > score2;}
    static float distance(const patch_descriptor &a, const patch_descriptor &b);
    float distance(float x, float y, const tracker::image& image) const;
};
