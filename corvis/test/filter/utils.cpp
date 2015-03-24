#include "gtest/gtest.h"
#include "util.h"
#include "filter.h"
#include "quaternion.h"
#include "utils.h"

static void check_initial_orientation_from_gravity(v4 gravity)
{
    v4 facing(0,1,0,0);
    v4 camera(0,0,-1,0), z(0,0,1,0), facing_perp(-facing[1], facing[0], 0, 0);
    quaternion q = initial_orientation_from_gravity(gravity);

    // gravity should point in the z direction
    test_v4_near(normalize(quaternion_rotate(q, gravity)), z, 2*F_T_EPS);

    // the camera should be aligned with facing
    EXPECT_GE(dot(quaternion_rotate(q, camera), facing), 0);

    // and have no conponent in the facing_perp direction
    EXPECT_NEAR(dot(quaternion_rotate(q, camera), facing_perp), 0, 2*F_T_EPS);
}

TEST(Filter, InitialOrientation)
{
    v4 gravities[] = {
        {0,    0,  9.8, 0},
        {0,    0, -9.8, 0},
        {0,  9.8,    0, 0},
        {0, -9.8,    0, 0},
        { 9.8, 0,    0, 0},
        {-9.8, 0,    0, 0},

        {0, .0001,  9.8, 0},
        {0,-.0001,  9.8, 0},
        { 0.001, 0, 9.8, 0},
        {-0.001, 0, 9.8, 0},

        { .01, .02, 9.8, 0},
        {-.01, .02, 9.8, 0},
        {-.01,-.02, 9.8, 0},
        { .01,-.02, 9.8, 0},

        { .1, .2, .3, 0},
        {-.1, .2, .3, 0},
        { .1,-.2, .3, 0},
        { .1, .2,-.3, 0},
        {-.1,-.2, .3, 0},
        { .1,-.2,-.3, 0},
        {-.1, .2,-.3, 0},
        {-.1,-.2,-.3, 0},
    };

    for (v4 g : gravities)
        check_initial_orientation_from_gravity(g);
}
