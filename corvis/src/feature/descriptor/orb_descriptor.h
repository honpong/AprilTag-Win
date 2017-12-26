/*********************************************************************
*  Loosely based on original ORB implementation by Willow Garage
*  modified by: Pedro Pinies, Lina Paz
*********************************************************************************/
#pragma once
#include "tracker.h"
#include "orb_constants.h"
#include <array>
#include <cmath>
#include <type_traits>

class orb_descriptor
{
public:
    static constexpr int L = orb_length;
    static constexpr int orb_half_patch_size = orb_half_patch_sz;
    static constexpr int border_size = orb_border_sz;
    static constexpr float max_track_distance = orb_max_track_distance;
    static constexpr float good_track_distance = orb_good_track_distance;

    struct raw : public array_ {
        static float distance(const orb_descriptor::raw &a, const orb_descriptor::raw &b);
        static raw mean(const std::vector<const raw*>& items);
    } descriptor;
    float cos_, sin_;

    orb_descriptor() {}
    orb_descriptor(const raw &d, float c, float s) : descriptor(d), cos_(c), sin_(s) {}
    orb_descriptor(float x, float y, const tracker::image& image);
    static float distance_reloc(const orb_descriptor &a, const orb_descriptor &b) {
        return raw::distance(a.descriptor, b.descriptor);
    }
    static float distance_stereo(const orb_descriptor &a, const orb_descriptor &b) {
        return raw::distance(a.descriptor, b.descriptor);
    }
    float distance_track(float x, float y, const tracker::image& image) const {
        return raw::distance(descriptor, orb_descriptor(x,y,image).descriptor);
    }

    float operator-(const orb_descriptor &o) const {
        return std::atan2(sin_ * o.cos_ - cos_ * o.sin_, cos_ * o.cos_ + sin_ * o.sin_);
    }

private:
    static const std::array<int, orb_half_patch_size + 1>& vUmax;

    static const std::array<int, orb_half_patch_size + 1> initialize_umax();
    void ic_angle(float x, float y, const tracker::image& image);
};
