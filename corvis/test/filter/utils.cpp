#include "gtest/gtest.h"
#include "util.h"
#include "filter.h"
#include "quaternion.h"
#include "utils.h"

static void check_initial_orientation_from_gravity(const v4 &gravity, const v4 &facing)
{
    v4 camera(0,0,1,0), z(0,0,1,0), facing_perp(-facing[1], facing[0], 0, 0);
    quaternion q = initial_orientation_from_gravity_facing(gravity, facing);

    // gravity should point in the z direction
    EXPECT_V4_NEAR((q * gravity).normalized(), z, 4*F_T_EPS);

    // the camera should be aligned with facing
    EXPECT_GE((q * camera).dot(facing), 0);

    // and have no conponent in the facing_perp direction
    EXPECT_NEAR((q * camera).dot(facing_perp), 0, 4*F_T_EPS);
}

TEST(Filter, InitialOrientation)
{
    f_t gravities[][3] = {
        {0,    0,  9.8},
        {0,    0, -9.8},
        {0,  9.8,    0},
        {0, -9.8,    0},
        { 9.8, 0,    0},
        {-9.8, 0,    0},

        {0, .0001,  9.8},
        {0,-.0001,  9.8},
        { 0.001, 0, 9.8},
        {-0.001, 0, 9.8},

        { .01, .02, 9.8},
        {-.01, .02, 9.8},
        {-.01,-.02, 9.8},
        { .01,-.02, 9.8},

        { .1, .2, .3},
        {-.1, .2, .3},
        { .1,-.2, .3},
        { .1, .2,-.3},
        {-.1,-.2, .3},
        { .1,-.2,-.3},
        {-.1, .2,-.3},
        {-.1,-.2,-.3},
    };

    for (f_t (&g)[3] : gravities)
        for (int i=0; i<10; i++)
            check_initial_orientation_from_gravity(v4(g[0],g[1],g[2],0), v4(cos(i*2*M_PI/10), sin(i*2*M_PI/10), 0, 0));
}
