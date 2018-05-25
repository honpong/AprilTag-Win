#include "gtest/gtest.h"
#include "estimate.h"
#include "util.h"

#include <random>
#include <algorithm>

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
    std::minstd_rand gen(100);
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
        for(int i = 0; i < d; i++) { v3 c(s( 6) + r(gen), s(7) + r(gen),  1 + r(gen)); v3 w = invert(g)*c; src.push_back(w); dst.push_back(c.head<2>()/c.z()); }

        auto seed = gen();
        { std::default_random_engine gen(seed); std::shuffle(src.begin(),src.end(),gen); }
        { std::default_random_engine gen(seed); std::shuffle(dst.begin(),dst.end(),gen); }

        transformation estimate;
        int max_iterations = 20;
        f_t max_reprojection_error = 150*F_T_EPS;
        f_t reprojection_error = estimate_transformation(src, dst, estimate, gen, max_iterations, max_reprojection_error);
        EXPECT_NEAR(reprojection_error, 0, max_reprojection_error);
        EXPECT_QUATERNION_NEAR(g.Q, estimate.Q, 25*F_T_EPS);
        EXPECT_V3_NEAR(g.T, estimate.T, std::max(g.T.norm()*0.005, F_T_EPS*4.));
    }
}

