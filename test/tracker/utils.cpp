#include <cmath>
#include "gtest/gtest.h"
#include "util.h"
#include "filter.h"
#include "quaternion.h"
#include "utils.h"

static void check_initial_orientation_from_gravity(const v3 &world_up,     const v3 &body_up,
                                                   const v3 &world_facing, const v3 &body_forward)
{
    quaternion q = initial_orientation_from_gravity_facing(world_up,     body_up,
                                                           world_facing, body_forward);

    // the rotation should align body_up exactly with world_up
    EXPECT_V3_NEAR((q * body_up).normalized(), world_up, 6*F_T_EPS);

    // The rotation should align body_forward in the general direction we should be facing
    EXPECT_GE((q * body_forward).dot(world_facing), 0);

    // such that there is no component in the third world direction
    EXPECT_NEAR((q * body_forward).dot(world_up.cross(world_facing)), 0, 6*F_T_EPS);
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
            check_initial_orientation_from_gravity(v3(0,0,1), v3(g[0],g[1],g[2]), v3(0,1,0), v3(cos(i*2*M_PI/10), sin(i*2*M_PI/10), 0));
}
