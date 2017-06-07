/********************************************************************************
Pedro Pinies, Lina Paz
*********************************************************************************/
#pragma once
#include "tracker.h"
#include <array>

class orb_descriptor
{
public:
    static const int L = 32; // descriptor length
    static const int orb_half_patch_size = 15;
    static const int border_size = 19;
    static constexpr float min_score = 200.f;
    static constexpr float good_score = 50.f;

    float angle;
    std::array<unsigned char, L> descriptor;

    static bool is_better(const float distance1, const float distance2) {return distance1 < distance2;}
    static double distance(const orb_descriptor &a, const orb_descriptor &b);
    void compute_descriptor(float x, float y, const tracker::image& image);

private:
    static int bit_pattern_31_[256 * 4];
    static const std::array<int, orb_half_patch_size + 2>& vUmax;

    template<int orb_half_patch_size_>
    static constexpr std::array<int, orb_half_patch_size_ + 2> initialize_umax();
    static float ic_angle(float x, float y, const tracker::image& image);
    static float atan2(float y, float x);
};
