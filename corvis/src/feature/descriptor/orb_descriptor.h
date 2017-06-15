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

    orb_descriptor(float x, float y, const tracker::image& image);
    static bool is_better(const float distance1, const float distance2) {return distance1 < distance2;}
    static double distance(const orb_descriptor &a, const orb_descriptor &b);
    double distance(float x, float y, const tracker::image& image) const {
        return orb_descriptor::distance(*this, orb_descriptor(x,y,image));
    }

private:
    static int bit_pattern_31_[256 * 4];
    static const std::array<int, orb_half_patch_size + 2>& vUmax;

    template<int orb_half_patch_size_>
    static constexpr std::array<int, orb_half_patch_size_ + 2> initialize_umax();
    static float ic_angle(float x, float y, const tracker::image& image);
    static float atan2(float y, float x);
};