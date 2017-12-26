/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2009, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************

* Authors: Ethan Rublee, Vincent Rabaud, Gary Bradski
*  modified by: Pedro Pinies, Lina Paz
*********************************************************************************/
#include "orb_descriptor.h"
#include <vector>
#include <iterator>
#include "tracker.h"
#include <iostream>
#include <stdint.h>
#include <climits>
#include <cmath>
#include "popcount.h"

#define lrintf [](float x) { return x >= 0 ? x + 0.5 : x - 0.5; }

 //
 // functions taken from OpenCV
 //

orb_descriptor::orb_descriptor(float x, float y, const tracker::image &image)
{
    ic_angle(x, y, image);
    const pattern_point* pattern = (const pattern_point*)orb_bit_pattern_31_;

    const int step = image.stride_px;
    const uint8_t* center = image.image + static_cast<int>(lrintf(y))*step
                                        + static_cast<int>(lrintf(x));

    uint8_t *d = reinterpret_cast<uint8_t*>(descriptor.data());

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

/* Return hamming distance between [a..a+255], [b..b+255].
 * psuedo code:
 * int distance = 0;
 * for (int i = 0..255)
 *  distance += xor(a[i], b[i])
 * return distance
 */

float orb_descriptor::raw::distance(const orb_descriptor::raw &a,
                                    const orb_descriptor::raw &b)
{
    orb_descriptor::raw::value_type dist = 0;
    for (auto p1 = a.begin(), p2 = b.begin(); p1 != a.end() && p2 != b.end(); p1++, p2++) {
        dist += popcount((*p1) ^ (*p2));
    }
    return static_cast<float>(dist);
}

orb_descriptor::raw orb_descriptor::raw::mean(
        const std::vector<const raw*>& items) {
    raw avg;
    avg.fill(0);
    if (items.empty()) return avg;

    std::array<size_t, sizeof(raw) * 8> counters = {};
    for (auto* item : items) {
        auto counter_it = counters.begin();
        for (auto pack : *item) {
            for (size_t i = 0; i < sizeof(pack) * 8; ++i, ++counter_it) {
                *counter_it += (pack & (decltype(pack)(1) << i)) >> i;
            }
        }
    }
    auto half = items.size() / 2;
    auto counter_it = counters.begin();
    for (auto& pack : avg) {
        using T = std::remove_reference<decltype(pack)>::type;
        for (size_t i = 0; i < sizeof(pack) * 8; ++i, ++counter_it) {
            pack |= static_cast<T>(*counter_it > half) << i;
        }
    }
    return avg;
}

const std::array<int, orb_descriptor::orb_half_patch_size + 1> orb_descriptor::initialize_umax()
{
    std::array<int, orb_half_patch_size + 1> umax;

    int vmax = static_cast<int>(std::floor(orb_half_patch_size * sqrtf(2) / 2 + 1));
    int vmin = static_cast<int>(std::ceil(orb_half_patch_size * sqrtf(2) / 2));
    for (int v = 0; v <= vmax; ++v)
    {
        umax[v] = static_cast<int>(lrintf(sqrtf((float)orb_half_patch_size * orb_half_patch_size - v * v)));
    }

    // Make sure we are symmetric
    for (int v = orb_half_patch_size, v0 = 0; v >= vmin; --v)
    {
        while (umax[v0] == umax[v0 + 1])
        {
            ++v0;
        }
        umax[v] = v0;
        ++v0;
    }
    return umax;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
const std::array<int, orb_descriptor::orb_half_patch_size + 1>& orb_descriptor::vUmax = orb_descriptor::initialize_umax();
#pragma GCC diagnostic pop

void orb_descriptor::ic_angle(float x, float y, const tracker::image& image)
{
    int m_01 = 0, m_10 = 0;

    const int step = image.stride_px;
    const uint8_t* center = image.image + static_cast<int>(lrintf(y)) * step
                                        + static_cast<int>(lrintf(x));

    // Treat the center line differently, v=0
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
    cos_ = d ? m_10*(1/d) : 1;
    sin_ = d ? m_01*(1/d) : 0;
}
