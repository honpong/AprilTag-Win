#include "gtest/gtest.h"
#include "estimate.h"
#include "util.h"

#include <random>
#include <algorithm>

TEST(Transformation, EstimateFlipTranslation)
{
    // Coordinates found from a mapper test. Produces an SVD when
    // estimating that (on mac) is a flip around Z rather than a
    // rotation
    transformation g_first(quaternion::Identity(), v3(-0.5,0.7,-4.3));
    transformation g_second(quaternion::Identity(), v3(-0.1, -1.2, -0.3));
    transformation g = g_second*invert(g_first);

    aligned_vector<v3> src;
    aligned_vector<v3> dst;
    for(int i = 0; i < 30; i++) {
        v3 position(i, i % 4, 2);
        position = g_first*position;
        v3 position2(i, i % 4, 2);
        position2 = g_second*position2;
        src.push_back(position);
        dst.push_back(position2);
    }

    transformation estimate;
    bool result = estimate_transformation(src, dst, estimate);
    EXPECT_TRUE(result);
    EXPECT_QUATERNION_NEAR(g.Q, estimate.Q, 0.001);
    EXPECT_V3_NEAR(g.T, estimate.T, std::max(g.T.norm()*0.001, F_T_EPS*4.));
}

TEST(Transformation, DegenerateLine)
{
    std::default_random_engine gen(200);
    std::uniform_real_distribution<float> r;

    aligned_vector<v3> src;
    aligned_vector<v3> dst;
    transformation g(quaternion(0.7, 0.6, 0.3, -0.2).normalized(), v3(-0.2,5.7,-4.2));
    for(int test = 0; test < 100; test++) {
        for(int i = 0; i < 10; i++) {
            float scale = 0.02;
            float point_scale = 2;
            v3 position(i*point_scale + r(gen)*scale, i * point_scale + r(gen)*scale, i * point_scale + r(gen)*scale);
            v3 position2 = g*position;
            src.push_back(position);
            dst.push_back(position2);
        }
        transformation estimate;
        bool result = estimate_transformation(src, dst, estimate);
        EXPECT_FALSE(result);
    }
}

TEST(Transformation, DegeneratePoint)
{
    std::default_random_engine gen(200);
    std::uniform_real_distribution<float> r;

    aligned_vector<v3> src;
    aligned_vector<v3> dst;
    for(int test = 0; test < 100; test++) {
        float t_scale = 2;
        float scale = F_T_EPS*1e3;
        transformation g(quaternion(r(gen), r(gen), r(gen), r(gen)).normalized(), v3(t_scale, t_scale, t_scale));
        for(int i = 0; i < 10; i++) {
            v3 position(r(gen)*scale, r(gen)*scale, r(gen)*scale);
            v3 position2 = g*position;
            src.push_back(position);
            dst.push_back(position2);
        }
        transformation estimate;
        bool result = estimate_transformation(src, dst, estimate);
        EXPECT_FALSE(result);
    }
}
