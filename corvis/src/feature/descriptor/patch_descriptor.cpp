/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
Pedro Pinies, Lina Paz

*********************************************************************************/
#include "patch_descriptor.h"
#include <cmath>

void patch_descriptor::compute_descriptor(float x, float y, const tracker::image &image) {
    for(int py = 0; py < full_patch_size; ++py) {
        for(int px = 0; px < full_patch_size; ++px) {
            descriptor[py * full_patch_size + px] = image.image[(int)x + px - half_patch_size +
                                                               ((int)y + py - half_patch_size) * image.stride_px];
        }
    }
}

double patch_descriptor::distance(const patch_descriptor::TDescriptor &a,
                                  const patch_descriptor::TDescriptor &b) {

    float sum_d1{0}, sum_d2{0}, sum_d1d2{0};
    float variance1{0}, variance2{0};
    for(int py = 0; py < full_patch_size; ++py) {
        for(int px = 0; px < full_patch_size; ++px) {
            uint8_t d1  = a[py * full_patch_size + px];
            uint8_t d2  = b[py * full_patch_size + px];
            // double-weighting the center on a 3x3 window
            int weight = ((std::fabs(px - half_patch_size) <=1) && (std::fabs(py - half_patch_size) <=1)) ? 2 : 1;
            sum_d1 += weight * d1;
            sum_d2 += weight * d2;
            sum_d1d2 += weight * d1 * d2;
            variance1 += weight * d1 * d1;
            variance2 += weight * d2 * d2;
        }
    }
    const int area = full_patch_size * full_patch_size + 3 * 3;
    float mean1 = sum_d1/area;
    float mean2 = sum_d2/area;
    variance1 = variance1 - area*mean1*mean1;
    variance2 = variance2 - area*mean2*mean2;
    float top = sum_d1d2 - mean2*sum_d1 - mean1*sum_d2 + mean1*mean2*area;

    // constant patches can't be matched
    if (variance1 < 1e-15 || variance2 < 1e-15 || top < 0.f)
        return min_score;

    return top*top/(variance1*variance2);
}

