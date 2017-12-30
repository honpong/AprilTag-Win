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
#include <cstdlib>
#include <cfloat>

patch_descriptor::patch_descriptor(float x, float y, const tracker::image &image) {
    uint8_t w[full_patch_size];
    for (int i=0; i<full_patch_size; i++)
        w[i] = std::abs(i - half_patch_size) <= 1 ? 2 : 1;

    for(int py = 0; py < full_patch_size; ++py) {
        for(int px = 0; px < full_patch_size; ++px) {
            uint8_t value = image.image[(int)x + px - half_patch_size +
                                       ((int)y + py - half_patch_size) * image.stride_px];
            descriptor[py * full_patch_size + px] = value;
            // double-weighting the center on a 3x3 window
            int weight = std::abs(py - half_patch_size) <= 1 ? w[px] : 1;
            mean += weight * value;
            variance += weight * value * value;
        }
    }
    const int area = full_patch_size * full_patch_size + 3 * 3;
    mean /= area;
    variance = variance - area*mean*mean;
}

patch_descriptor::patch_descriptor(const std::array<unsigned char, L> &d) : descriptor(d) {
    uint8_t w[full_patch_size];
    for (int i=0; i<full_patch_size; i++)
        w[i] = std::abs(i - half_patch_size) <= 1 ? 2 : 1;

    for(int py = 0; py < full_patch_size; ++py) {
        for(int px = 0; px < full_patch_size; ++px) {
            uint8_t value = d[py * full_patch_size + px];
            // double-weighting the center on a 3x3 window
            int weight = std::abs(py - half_patch_size) <= 1 ? w[px] : 1;
            mean += weight * value;
            variance += weight * value * value;
        }
    }
    const int area = full_patch_size * full_patch_size + 3 * 3;
    mean /= area;
    variance = variance - area*mean*mean;
}

float patch_descriptor::distance(const patch_descriptor &a,
                                 const patch_descriptor &b) {
    // constant patches can't be matched
    if (a.variance < FLT_EPSILON || b.variance < FLT_EPSILON)
        return INFINITY;

    uint8_t w[full_patch_size];
    for (int i=0; i<full_patch_size; i++)
        w[i] = std::abs(i - half_patch_size) <= 1 ? 2 : 1;

    float distance = 0;
    for(int py = 0; py < full_patch_size; ++py) {
        for(int px = 0; px < full_patch_size; ++px) {
            int index = py * full_patch_size + px;
            float value =  (a.descriptor[index]-a.mean)*(b.descriptor[index]-b.mean);
            // double-weighting the center on a 3x3 window
            int weight = std::abs(py - half_patch_size) <= 1 ? w[px] : 1;
            distance += weight*value;
        }
    }

    if (distance < 0.f)
        return INFINITY;

    return 1 - distance*distance/(a.variance*b.variance);
}

float patch_descriptor::distance(float x, float y, const tracker::image &image) const {
    uint8_t w[full_patch_size];
    for (int i=0; i<full_patch_size; i++)
        w[i] = std::abs(i - half_patch_size) <= 1 ? 2 : 1;

    float sum_d2 = 0, sum_d1d2 = 0, variance2 = 0;
    for(int py = 0; py < full_patch_size; ++py) {
        for(int px = 0; px < full_patch_size; ++px) {
            uint8_t d1  = descriptor[py * full_patch_size + px];
            uint8_t d2  = image.image[(int)x + px - half_patch_size +
                                      ((int)y + py - half_patch_size) * image.stride_px];
            // double-weighting the center on a 3x3 window
            int weight = std::abs(py - half_patch_size) <= 1 ? w[px] : 1;
            sum_d2 += weight * d2;
            sum_d1d2 += weight * d1 * d2;
            variance2 += weight * d2 * d2;
        }
    }

    const int area = full_patch_size * full_patch_size + 3 * 3;
    float mean2 = sum_d2/area;
    variance2 = variance2 - area*mean2*mean2;
    float top = sum_d1d2 - mean2*(mean*area) - mean*sum_d2 + mean*mean2*area;

    // constant patches can't be matched
    if (variance < FLT_EPSILON || variance2 < FLT_EPSILON || top < 0.f)
        return INFINITY;

    return 1 - top*top/(variance*variance2);
}
