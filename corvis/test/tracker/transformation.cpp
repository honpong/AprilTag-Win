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
        transformation g(quaternion(r(gen), r(gen), r(gen), r(gen)).normalized(), v3(r(gen),r(gen),r(gen)));
        aligned_vector<v3> src;
        aligned_vector<v3> dst;
        for(int i = 0; i < 20; i++) {
            v3 position(r(gen), r(gen), r(gen));
            v3 position2 = g*position;
            src.push_back(position);
            dst.push_back(position2);
        }

        transformation estimate;
        bool result = estimate_transformation(src, dst, estimate);
        EXPECT_TRUE(result);
        EXPECT_QUATERNION_NEAR(g.Q, estimate.Q, 0.001);
        EXPECT_V3_NEAR(g.T, estimate.T, std::max(g.T.norm()*0.001, F_T_EPS*4.));
    }
}

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

TEST(Transformation, EPnP)
{
    std::default_random_engine gen(100);
    std::uniform_real_distribution<float> r;
    std::uniform_int_distribution<int> b(0,1);
    auto s = [&b,&gen](float n) { return (b(gen) ? -1 : 1) * n; };

    for(int a = 1; a < 10; a++)
    for(int b = 4; b < 10; b++)
    for(int c = 1; c < 10; c++) {
        transformation g(quaternion(r(gen), r(gen), r(gen), r(gen)).normalized(), v3(r(gen),r(gen),r(gen)));
        aligned_vector<v3> src;
        aligned_vector<v2> dst;
        for(int i = 0; i < a; i++) { v3 c(s( 1) + r(gen), s(3) + r(gen), 20 + r(gen)); v3 w = invert(g)*c; src.push_back(w); dst.push_back(c.head<2>()/c.z()); }
        for(int i = 0; i < b; i++) { v3 c(s( 3) + r(gen), s(2) + r(gen),  8 + r(gen)); v3 w = invert(g)*c; src.push_back(w); dst.push_back(c.head<2>()/c.z()); }
        for(int i = 0; i < c; i++) { v3 c(s(10) + r(gen), s(5) + r(gen),  3 + r(gen)); v3 w = invert(g)*c; src.push_back(w); dst.push_back(c.head<2>()/c.z()); }

        transformation estimate;
        f_t reprojection_error = estimate_transformation(src, dst, estimate);
        EXPECT_NEAR(reprojection_error, 0, 100*F_T_EPS);
        EXPECT_QUATERNION_NEAR(g.Q, estimate.Q, 25*F_T_EPS);
        EXPECT_V3_NEAR(g.T, estimate.T, std::max(g.T.norm()*0.005, F_T_EPS*4.));
    }
}

TEST(Transformation, EPnPRansac)
{
    std::default_random_engine gen(100);
    std::uniform_real_distribution<float> r;
    std::uniform_int_distribution<int> b(0,1);
    auto s = [&b,&gen](float n) { return (b(gen) ? -1 : 1) * n; };

    for(int a = 4; a < 10; a++)
    for(int b = 4; b < 10; b++)
    for(int c = 4; c < 10; c++)
    for(int d = 3; d < 4; d++) {
        transformation g(quaternion(r(gen), r(gen), r(gen), r(gen)).normalized(), v3(r(gen),r(gen),r(gen)));
        aligned_vector<v3> src;
        aligned_vector<v2> dst;
        for(int i = 0; i < a; i++) { v3 c(s( 1) + r(gen), s(3) + r(gen), 20 + r(gen)); v3 w = invert(g)*c; src.push_back(w); dst.push_back(c.head<2>()/c.z()); }
        for(int i = 0; i < b; i++) { v3 c(s( 3) + r(gen), s(2) + r(gen),  8 + r(gen)); v3 w = invert(g)*c; src.push_back(w); dst.push_back(c.head<2>()/c.z()); }
        for(int i = 0; i < c; i++) { v3 c(s(10) + r(gen), s(5) + r(gen),  3 + r(gen)); v3 w = invert(g)*c; src.push_back(w); dst.push_back(c.head<2>()/c.z()); }
        for(int i = 0; i < d; i++) { v3 c(s( 6) + r(gen), s(7) + r(gen),  1 + r(gen)); v3 w =           c; src.push_back(w); dst.push_back(c.head<2>()/c.z()); }

        auto seed = gen();
        { std::default_random_engine gen(seed); std::shuffle(src.begin(),src.end(),gen); }
        { std::default_random_engine gen(seed); std::shuffle(dst.begin(),dst.end(),gen); }

        transformation estimate;
        f_t reprojection_error = estimate_transformation(src, dst, estimate, gen);
        EXPECT_NEAR(reprojection_error, 0, 100*F_T_EPS);
        EXPECT_QUATERNION_NEAR(g.Q, estimate.Q, 25*F_T_EPS);
        EXPECT_V3_NEAR(g.T, estimate.T, std::max(g.T.norm()*0.005, F_T_EPS*4.));
    }
}
