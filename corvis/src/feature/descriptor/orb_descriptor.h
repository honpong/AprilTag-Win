/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2016 Intel Corporation. All Rights Reserved.

*********************************************************************************/
#pragma once
#include "tracker.h"
#include <array>

class orb_descriptor
{
public:
    static const int L = 32; // descriptor length
    static const int orb_half_patch_size = 15;
    static const int orb_half_patch_diagonal = 19;

    typedef std::array<unsigned char, L> TDescriptor;
    typedef const TDescriptor *pDescriptor;

    TDescriptor descriptor;
    float angle;

    static double distance(const TDescriptor &a, const TDescriptor &b);
    // Functions below are taken from OpenCV to compute the orientations
    void compute_descriptor(float x, float y, const tracker::image& image);

private:
    static int bit_pattern_31_[256 * 4];
    static const std::array<int, orb_half_patch_size + 2>& vUmax;

    template<int orb_half_patch_size_>
    static constexpr std::array<int, orb_half_patch_size_ + 2> initialize_umax();
    static float ic_angle(float x, float y, const tracker::image& image);
    static float atan2(float y, float x);
};
