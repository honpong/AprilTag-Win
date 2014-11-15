#ifndef __UTIL_H
#define __UTIL_H

#include "gtest/gtest.h"
#include "cor_types.h"
#include "vec4.h"
#include "quaternion.h"

static inline void test_m4_near(const m4 &a, const m4 &b, const f_t bounds)
{
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            EXPECT_NEAR(a[i][j], b[i][j], bounds) << "Where i is " << i << " and j is " << j;
        }
    }
}

static inline void test_v4_near(const v4 &a, const v4 &b, const f_t bounds)
{
    for(int i = 0; i < 4; ++i) {
        EXPECT_NEAR(a[i], b[i], bounds) << "Where i is " << i;
    }
}

static inline void test_quaternion_near(const quaternion &a, const quaternion &b, const f_t bounds)
{
    EXPECT_NEAR(a.w(), b.w(), bounds) << "Where index is w";
    EXPECT_NEAR(a.x(), b.x(), bounds) << "Where index is x";
    EXPECT_NEAR(a.y(), b.y(), bounds) << "Where index is y";
    EXPECT_NEAR(a.z(), b.z(), bounds) << "Where index is z";
}

#endif