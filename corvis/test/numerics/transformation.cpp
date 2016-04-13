#include "gtest/gtest.h"
#include "transformation.h"
#include "util.h"

#include <random>
#include <algorithm>

TEST(Transformation, Estimate)
{
    std::default_random_engine gen(100);
    std::uniform_real_distribution<float> r;

    for(int t = 0; t < 200; t++) {
        transformation g(normalize(quaternion(r(gen), r(gen), r(gen), r(gen))), v4(r(gen),r(gen),r(gen),0));
        aligned_vector<v4> src;
        aligned_vector<v4> dst;
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

    aligned_vector<v4> src;
    aligned_vector<v4> dst;
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

TEST(Transformation, DegenerateLine)
{
    std::default_random_engine gen(200);
    std::uniform_real_distribution<float> r;

    aligned_vector<v4> src;
    aligned_vector<v4> dst;
    transformation g(normalize(quaternion(0.7, 0.6, 0.3, -0.2)), v4(-0.2,5.7,-4.2,0));
    for(int test = 0; test < 100; test++) {
        for(int i = 0; i < 10; i++) {
            float scale = 0.02;
            float point_scale = 2;
            v4 position(i*point_scale + r(gen)*scale, i * point_scale + r(gen)*scale, i * point_scale + r(gen)*scale, 0);
            v4 position2 = g*position;
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

    aligned_vector<v4> src;
    aligned_vector<v4> dst;
    for(int test = 0; test < 100; test++) {
        float t_scale = 2;
        float scale = F_T_EPS*1e3;
        transformation g(normalize(quaternion(r(gen), r(gen), r(gen), r(gen))), v4(t_scale, t_scale, t_scale,0));
        for(int i = 0; i < 10; i++) {
            v4 position(r(gen)*scale, r(gen)*scale, r(gen)*scale, 0);
            v4 position2 = g*position;
            src.push_back(position);
            dst.push_back(position2);
        }
        transformation estimate;
        bool result = estimate_transformation(src, dst, estimate);
        EXPECT_FALSE(result);
    }
}
