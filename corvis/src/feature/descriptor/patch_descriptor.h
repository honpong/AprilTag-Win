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
    static const int border_size = half_patch_size;
    static constexpr float min_score = 0.2f*0.2f;
    static constexpr float good_score = 0.65f*0.65f;

    std::array<unsigned char, L> descriptor;
    float mean{0}, variance{0};

    static bool is_better(const float score1, const float score2) {return score1 > score2;}
    static double distance(const patch_descriptor &a, const patch_descriptor &b);
    double distance(float x, float y, const tracker::image& image) const;
    void compute_descriptor(float x, float y, const tracker::image& image);
};
