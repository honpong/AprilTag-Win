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

TEST(Transformation, TransformationCovComposition)
{
    const int num_tests = 100;
    const float eps = 1e-4; // step for numerical approximation
    float J1_J1T_err = 0.f;
    float J2_J2T_err = 0.f;
    for(int test = 0; test < num_tests; ++test) {
        transformation G1(rotation_vector(v3::Random()), v3::Random());
        transformation G2(rotation_vector(v3::Random()), v3::Random());

        transformation_cov G1_cov0(G1, m<6,6>::Zero());
        transformation_cov G1_covI(G1, m<6,6>::Identity());
        transformation_cov G2_cov0(G2, m<6,6>::Zero());
        transformation_cov G2_covI(G2, m<6,6>::Identity());

        // Calculate numerical jacobians J1n and J2n
        const transformation& G1G2 = G1*G2;
        m<6,6> J1n;
        m<6,6> J2n;
        for(int i = 0; i < 6; ++i) {
            v<6> dphi_t(v<6>::Zero());
            dphi_t[i] = eps;
            transformation G1e(to_quaternion(rotation_vector(dphi_t.tail(3)))*G1.Q, G1.T + dphi_t.head(3));
            transformation G1eG2 = G1e*G2;
            J1n.block<3,1>(0,i) = (G1eG2.T-G1G2.T)/eps;
            J1n.block<3,1>(3,i) = to_rotation_vector(G1eG2.Q*G1G2.Q.inverse()).raw_vector()/eps;

            transformation G2e(to_quaternion(rotation_vector(dphi_t.tail(3)))*G2.Q, G2.T + dphi_t.head(3));
            transformation G1G2e = G1*G2e;
            J2n.block<3,1>(0,i) = (G1G2e.T-G1G2.T)/eps;
            J2n.block<3,1>(3,i) = to_rotation_vector(G1G2e.Q*G1G2.Q.inverse()).raw_vector()/eps;
        }

        // Testing J1*J1^T vs numerical jacobian
        transformation_cov G1G2_cov = G1_covI*G2_cov0; // the cov of this is = J1*J1^T
        J1_J1T_err = fmax(J1_J1T_err, (G1G2_cov.cov-J1n*J1n.transpose()).array().abs().maxCoeff());

        // Testing J2*J2^T vs numerical jacobian
        G1G2_cov = G1_cov0*G2_covI; // the cov of this is = J2*J2^T
        J2_J2T_err = fmax(J2_J2T_err, (G1G2_cov.cov-J2n*J2n.transpose()).array().abs().maxCoeff());
    }
    EXPECT_NEAR(J1_J1T_err, 0, 1e-2); // with double precission this can be reduced to 1e-8
    EXPECT_NEAR(J2_J2T_err, 0, 1e-2); // with double precission this can be reduced to 1e-8
}

TEST(Transformation, TransformationCovInverse)
{
    const int num_tests = 100;
    const float eps = 1e-4; // step for numerical approximation
    float Jinv_JinvT_err = 0.f;
    for(int test = 0; test < num_tests; ++test) {
        transformation G12(rotation_vector(v3::Random()), v3::Random());
        transformation_cov G12_cov(G12, m<6,6>::Identity());
        transformation_cov G21_cov = invert(G12_cov); // cov is = Jinv*Jinv^T

        const transformation& G21 = G21_cov.G;
        // Calculate numerical jacobian Jinv
        m<6,6> Jinv;
        for(int i = 0; i < 6; ++i) {
            v<6> dphi_t(v<6>::Zero());
            dphi_t[i] = eps;
            transformation G21e = invert(transformation(to_quaternion(rotation_vector(dphi_t.tail(3)))*G12.Q, G12.T + dphi_t.head(3)));
            Jinv.block<3,1>(0,i) = (G21e.T-G21.T)/eps;
            Jinv.block<3,1>(3,i) = to_rotation_vector(G21e.Q*G21.Q.inverse()).raw_vector()/eps;
        }

        // Testing Jinv*Jinv^T vs numerical jacobian
        Jinv_JinvT_err = fmax(Jinv_JinvT_err, (G21_cov.cov-Jinv*Jinv.transpose()).array().abs().maxCoeff());
    }
    EXPECT_NEAR(Jinv_JinvT_err, 0, 1e-2); // with double precission this can be reduced to 1e-8
}

TEST(Transformation, TransformationCovCompose)
{
    const int num_tests = 100;
    f_t cov_err = 0.f;
    for(int test = 0; test < num_tests; ++test) {
        // input data
        m<12,12> cov = m<12,12>::Random();
        cov = cov.transpose()*cov;

        transformation_cov G1(transformation(rotation_vector(v3::Random()), v3::Random()), cov.block<6,6>(0,0));
        transformation_cov G2(transformation(rotation_vector(v3::Random()), v3::Random()), cov.block<6,6>(6,6));

        const auto& cov12 = cov.block<6,6>(0,6);

        // jacobians
        m3 R1 = G1.G.Q.toRotationMatrix();
        m3 S = -skew(G1.G.Q * G2.G.T);

        m<6,6> J1(m<6,6>::Identity());
        J1.block<3,3>(0,3) = S;

        m<6,6> J2(m<6,6>::Zero());
        J2.block<3,3>(0,0) = R1;
        J2.block<3,3>(3,3) = R1;

        m<6,12> J;
        J.block<6,6>(0,0) = J1;
        J.block<6,6>(0,6) = J2;

        // calculate covariance with correlation
        m<6,6> cov_gt = J*cov*J.transpose(); // analytical
        transformation_cov G3 = compose(G1, G2, cov12);

        // test
        cov_err = fmax(cov_err, (cov_gt - G3.cov).array().abs().maxCoeff());
    }
    EXPECT_NEAR(cov_err, 0, 1e-5);
}
