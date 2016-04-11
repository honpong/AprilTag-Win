#include "gtest/gtest.h"
#include "transformation.h"
#include "util.h"

#include <random>
#include <algorithm>
#include <vector>

TEST(Transformation, Estimate)
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
        bool result = estimate_transformation(src, dst, estimate);
        EXPECT_TRUE(result);
        EXPECT_QUATERNION_NEAR(g.Q, estimate.Q, 0.001);
        EXPECT_V4_NEAR(g.T, estimate.T, std::max(g.T.norm()*0.001, F_T_EPS*4.));
    }
}

TEST(Transformation, EstimateFlipTranslation)
{
    // Coordinates found from a mapper test. Produces an SVD when
    // estimating that (on mac) is a flip around Z rather than a
    // rotation
    transformation g_first(quaternion(), v4(-0.5,0.7,-4.3,0));
    transformation g_second(quaternion(), v4(-0.1, -1.2, -0.3, 0));
    transformation g = g_second*invert(g_first);

    std::vector<v4> src;
    std::vector<v4> dst;
    for(int i = 0; i < 30; i++) {
        v4 position(i, i % 4, 2, 0);
        position = g_first*position;
        v4 position2(i, i % 4, 2, 0);
        position2 = g_second*position2;
        src.push_back(position);
        dst.push_back(position2);
    }

    transformation estimate;
    bool result = estimate_transformation(src, dst, estimate);
    EXPECT_TRUE(result);
    EXPECT_QUATERNION_NEAR(g.Q, estimate.Q, 0.001);
    EXPECT_V4_NEAR(g.T, estimate.T, std::max(g.T.norm()*0.001, F_T_EPS*4.));
}