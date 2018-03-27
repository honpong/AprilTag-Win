#pragma once
#include <moviVectorTypes.h>
#include <array>
#include <cmath>
#include "tracker.h"
#include "orb_descriptor.h"

class orb_shave : public orb_descriptor {
public:
    orb_shave(float x, float y,  const tracker::image &image, orb_data* output_descriptor);
    static const std::array<int, orb_half_patch_size + 1>& vUmax;

private:
    void binary_test(uchar *center, int step, uchar *d);
    void ic_angle(float x, float y, const int step, const uint8_t image[], int vUmax[], int orb_half_patch_size, float *sin_, float *cos_);
};
