#ifndef __UTIL_H
#define __UTIL_H

#include "vec4.h"
#include "quaternion.h"
#include "transformation.h"
#include "gtest/gtest.h"

#undef GTEST_DISALLOW_ASSIGN_
#define GTEST_DISALLOW_ASSIGN_(type) public: EIGEN_MAKE_ALIGNED_OPERATOR_NEW void operator=(type const &)

::testing::AssertionResult
test_m4_near(const char* expr1, const char* expr2, const char* bounds_expr,
             const m4 &a, const m4 &b, const f_t bounds);

#define EXPECT_M4_NEAR(a,b,bounds) EXPECT_PRED_FORMAT3(test_m4_near, a, b, bounds)

::testing::AssertionResult
test_m3_near(const char* expr1, const char* expr2, const char* bounds_expr,
             const m3 &a, const m3 &b, const f_t bounds);

#define EXPECT_M3_NEAR(a,b,bounds) EXPECT_PRED_FORMAT3(test_m3_near, a, b, bounds)

::testing::AssertionResult
test_v4_near(const char* expr1, const char* expr2, const char* bounds_expr,
             const v4 &a, const v4 &b, const f_t bounds);

#define EXPECT_V4_NEAR(a,b,bounds) EXPECT_PRED_FORMAT3(test_v4_near, a, b, bounds)

::testing::AssertionResult
test_v3_near(const char* expr1, const char* expr2, const char* bounds_expr,
             const v3 &a, const v3 &b, const f_t bounds);

#define EXPECT_V3_NEAR(a,b,bounds) EXPECT_PRED_FORMAT3(test_v3_near, a, b, bounds)

::testing::AssertionResult
test_quaternion_near(const char* expr1, const char* expr2, const char* bounds_expr,
                     const quaternion &a, const quaternion &b, const f_t bounds);

#define EXPECT_QUATERNION_NEAR(a,b,bounds) EXPECT_PRED_FORMAT3(test_quaternion_near, a, b, bounds)

::testing::AssertionResult
test_rotation_vector_near(const char* expr1, const char* expr2, const char* bounds_expr,
                          const rotation_vector &a, const rotation_vector &b, const f_t bounds);

#define EXPECT_ROTATION_VECTOR_NEAR(a,b,bounds) EXPECT_PRED_FORMAT3(test_rotation_vector_near, a, b, bounds)

static inline ::testing::AssertionResult
test_quaternion_rotation_near(const char* expr1, const char* expr2, const char* bounds_expr,
                              const quaternion &a, const quaternion &b, const f_t bounds)
{
    //interpreted as rotations, q = -q
    if((a.w() < 0. && b.w() >= 0.) || (a.w() >= 0. && b.w() < 0.))
        return test_quaternion_near(expr1, expr2, bounds_expr, a, quaternion(-b.w(), -b.x(), -b.y(), -b.z()), bounds);
    else
        return test_quaternion_near(expr1, expr2, bounds_expr, a, b, bounds);
}

#define EXPECT_QUATERNION_ROTATION_NEAR(a,b,bounds) EXPECT_PRED_FORMAT3(test_quaternion_rotation_near, a, b, bounds)

::testing::AssertionResult
test_transformation_near(const char* expr1, const char* expr2, const char* bounds_expr,
                         const transformation &a, const transformation &b, const f_t bounds);

#define EXPECT_TRANSFORMATION_NEAR(a,b,bounds) EXPECT_PRED_FORMAT3(test_transformation_near, a, b, bounds)

#endif
