/*********************************************************************
*  Loosely based on original ORB implementation by Willow Garage
*  modified by: Pedro Pinies, Lina Paz
*********************************************************************************/
#pragma once
#include "tracker.h"
#include <array>
#include <cmath>

class orb_descriptor
{
public:
    static constexpr int L = 32; // descriptor length
    static constexpr int orb_half_patch_size = 15;
    static constexpr int border_size = 19;
    static constexpr float bad_score = INFINITY;
    static constexpr float min_score = 200.f;
    static constexpr float good_score = 50.f;

    struct raw : public std::array<uint64_t, L/8> {
        static float distance(const orb_descriptor::raw &a, const orb_descriptor::raw &b);
        static raw mean(const std::vector<const raw*>& items);
    } descriptor;
    float cos_, sin_;

    orb_descriptor(const raw &d, float c, float s) : descriptor(d), cos_(c), sin_(s) {}
    orb_descriptor(float x, float y, const tracker::image& image);
    static bool is_better(const float distance1, const float distance2) {return distance1 < distance2;}
    static float distance(const orb_descriptor &a, const orb_descriptor &b) {
        return raw::distance(a.descriptor, b.descriptor);
    }
    float distance(float x, float y, const tracker::image& image) const {
        return orb_descriptor::distance(*this, orb_descriptor(x,y,image));
    }

    float operator-(const orb_descriptor &o) const {
        return std::atan2(sin_ * o.cos_ - cos_ * o.sin_, cos_ * o.cos_ + sin_ * o.sin_);
    }

private:
    static const int bit_pattern_31_[256 * 4];
    static const std::array<int, orb_half_patch_size + 1>& vUmax;

    static const std::array<int, orb_half_patch_size + 1> initialize_umax();
    void ic_angle(float x, float y, const tracker::image& image);
};
