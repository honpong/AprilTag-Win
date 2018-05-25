#include "gtest/gtest.h"
#include "transformation_cov.h"
#include "util.h"

#include <random>
#include <algorithm>

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
