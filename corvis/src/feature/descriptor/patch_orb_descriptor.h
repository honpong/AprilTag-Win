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
#include "patch_descriptor.h"
#include "orb_descriptor.h"

class patch_orb_descriptor
{
public:
    static constexpr float max_track_distance = patch_descriptor::max_track_distance;
    static constexpr float good_track_distance = patch_descriptor::good_track_distance;
    static constexpr int border_size = patch_descriptor::border_size;
    orb_descriptor orb;
    patch_descriptor patch;
    bool orb_computed {false};
    patch_orb_descriptor(float x, float y, const tracker::image& image): patch(x, y, image), orb_computed(false) {}
    patch_orb_descriptor(const patch_descriptor &p): patch(p), orb_computed(false) {}
    static float distance_reloc(const patch_orb_descriptor &a, const patch_orb_descriptor &b)
    {
        return a.orb_computed && b.orb_computed ?
        orb_descriptor::distance_reloc(a.orb, b.orb) :
        patch_descriptor::distance_reloc(a.patch, b.patch);
    }
    static float distance_stereo(const patch_orb_descriptor &a, const patch_orb_descriptor &b)
    { return patch_descriptor::distance_stereo(a.patch, b.patch); }
    float distance_track(float x, float y, const tracker::image& image) const
    { return patch.distance_track(x, y, image); }
};
