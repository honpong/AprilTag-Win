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
    static constexpr int L = full_patch_width * full_patch_width; // descriptor length
    static constexpr int full_patch_size = full_patch_width;
    static constexpr int half_patch_size = half_patch_width;
    static constexpr int border_size = half_patch_size;
    static constexpr float max_track_distance = patch_max_track_distance;
    static constexpr float good_track_distance = patch_good_track_distance;

    std::array<unsigned char, L> descriptor;
    float mean{0}, variance{0};

    patch_descriptor() {}
    patch_descriptor(float x, float y, const tracker::image& image);
    patch_descriptor(const std::array<unsigned char, L> &d);
    static float distance_reloc(const patch_descriptor &a, const patch_descriptor &b) { return  distance_stereo(a, b); }
    static float distance_stereo(const patch_descriptor &a, const patch_descriptor &b);
    float distance_track(float x, float y, const tracker::image& image) const;
};
