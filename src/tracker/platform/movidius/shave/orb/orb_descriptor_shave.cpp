#include "orb_descriptor_shave.h"
#include <math.h>
#include <stdio.h>
#include <mv_types.h>

#define lrintf [](float x) { return x >= 0 ? x + 0.5 : x - 0.5; }

extern "C" void binary_test_asm(uint8_t *center, int step, float sin_, float cos_, float* pattern, uchar *descriptor, int dsize);
extern "C" void ic_angle_asm(float x, float y, int step, u8 image[], int vUmax[], int orb_half_patch_size, float *sin_, float *cos_);

orb_shave::orb_shave(float x, float y,  const tracker::image &image, orb_data* output_descriptor)
{
    ic_angle_asm((float)x, (float)y, image.stride_px, (uint8_t*)image.image, (int *)vUmax.data(),orb_half_patch_size, &sin_, &cos_);
    //ic_angle((float)x, (float)y, image.stride_px, (uint8_t*)image.image, (int *)vUmax.data(),orb_half_patch_size, &sin_, &cos_);

    uint8_t* center = (uint8_t*)image.image + static_cast<int>(lrintf(y))*image.stride_px
                                            + static_cast<int>(lrintf(x));

    uint8_t* d = reinterpret_cast<uint8_t*>(descriptor.data());
    int dsize = (int)sizeof(descriptor);

    binary_test_asm(center, image.stride_px, sin_, cos_, (float*)&orb_bit_pattern_31_, d, dsize);
   // binary_test(center, dsize, d);

    output_descriptor->d = descriptor;
    output_descriptor->sin_ = sin_;
    output_descriptor->cos_ = cos_;
}

void orb_shave::ic_angle(float x, float y, const int step, const u8 image[], int vUmax[], int orb_half_patch_size, float *sin_, float *cos_)
{
    int m_01 = 0, m_10 = 0;
    x = (x >= 0) ? (x + 0.5f) : (x - 0.5f);
    y = (y >= 0) ? (y + 0.5f) : (y - 0.5f);
    int ix = (int)x;
    int iy = (int)y;
    const u8 *center = image + iy * step + ix;

    for (int u = -orb_half_patch_size; u <= orb_half_patch_size; ++u)
    {
        m_10 += u * center[u];
    }
    // Go line by line in the circular patch
    const int step1 = step/sizeof(uint8_t);
    for (int v = 1; v <= orb_half_patch_size; ++v)
    {
        // Proceed over the two lines
        int v_sum = 0;
        int d = vUmax[v];
        for (int u = -d; u <= d; ++u)
        {
            int val_plus = center[u + v*step1];
            int val_minus = center[u - v*step1];
            v_sum += (val_plus - val_minus);
            m_10 += u * (val_plus + val_minus);
        }
        m_01 += v * v_sum;
    }

    float d = std::hypot((float)m_01,(float)m_10);
    *cos_ = d ? m_10*(1/d) : 1;
    *sin_ = d ? m_01*(1/d) : 0;
}

void orb_shave::binary_test(uchar *center, int step, uchar *d) {

    const pattern_point* pattern = (const pattern_point*)orb_bit_pattern_31_;

#define GET_VALUE(idx) \
    center[static_cast<int>(lrintf(pattern[idx].x*sin_ + pattern[idx].y*cos_))*step + \
           static_cast<int>(lrintf(pattern[idx].x*cos_ - pattern[idx].y*sin_))]

    for (size_t i = 0; i < sizeof(descriptor); ++i, pattern += 16)
    {
        int t0, t1, val;
        t0 = GET_VALUE(0);
        t1 = GET_VALUE(1);
        val = t0 < t1;
        t0 = GET_VALUE(2);
        t1 = GET_VALUE(3);
        val |= (t0 < t1) << 1;
        t0 = GET_VALUE(4);
        t1 = GET_VALUE(5);
        val |= (t0 < t1) << 2;
        t0 = GET_VALUE(6);
        t1 = GET_VALUE(7);
        val |= (t0 < t1) << 3;
        t0 = GET_VALUE(8);
        t1 = GET_VALUE(9);
        val |= (t0 < t1) << 4;
        t0 = GET_VALUE(10);
        t1 = GET_VALUE(11);
        val |= (t0 < t1) << 5;
        t0 = GET_VALUE(12);
        t1 = GET_VALUE(13);
        val |= (t0 < t1) << 6;
        t0 = GET_VALUE(14);
        t1 = GET_VALUE(15);
        val |= (t0 < t1) << 7;
        d[i] = (unsigned char)val;
    }

#undef GET_VALUE
}
const std::array<int, orb_shave::orb_half_patch_size + 1>& orb_shave::vUmax = { {15, 15, 15, 15, 14, 14, 14, 13, 13, 12, 11, 10, 9, 8, 6, 3} };
