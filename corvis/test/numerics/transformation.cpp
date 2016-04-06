#include "gtest/gtest.h"
#include "transformation.h"
#include "util.h"

#include <random>
#include <algorithm>
#include <vector>

TEST(Transformation, EstimateRigid)
{
    std::default_random_engine gen(100);
    std::uniform_real_distribution<float> r;

    for(int t = 0; t < 200; t++) {
        transformation g(normalize(quaternion(r(gen), r(gen), r(gen), r(gen))), v4(r(gen),r(gen),r(gen),0));
        std::vector<v4> src;
        std::vector<v4> dst;
        for(int i = 0; i < 20; i++) {
            v4 position(r(gen), r(gen), r(gen), 0);
            v4 position2 = g*position;
            src.push_back(position);
            dst.push_back(position2);
        }

        transformation estimate;
        bool result = estimate_rigid_transformation(src, dst, estimate);
        EXPECT_TRUE(result);
        EXPECT_QUATERNION_NEAR(g.Q, estimate.Q, 0.001);
        EXPECT_V4_NEAR(g.T, estimate.T, std::max(g.T.norm()*0.001, F_T_EPS*4.));
    }
}
