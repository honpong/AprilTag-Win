#include "gtest/gtest.h"
#include "estimate.h"
#include "util.h"

#include <random>
#include <algorithm>

TEST(FivePoint, FivePoints)
{
    std::default_random_engine gen(100);
    std::uniform_real_distribution<f_t> r;
    constexpr size_t num_points = 5;
    constexpr size_t num_tests = 100;

    std::vector<f_t> reprojection_errors(num_tests);
    for(size_t test = 0; test < num_tests; test++) {
        // simulation parameters
        f_t scene_depth = 2*r(gen);
        f_t scene_distance = 1;
        f_t scene_width = 2*r(gen);
        f_t baseline = r(gen) + 1;
        v3 motion_direction{r(gen)-0.5f, r(gen)-0.5f, r(gen)};
        motion_direction /= motion_direction.norm();

        // generate 3D points
        Eigen::Matrix<f_t, 3, num_points> P3d_1 = Eigen::Matrix<f_t, 3, num_points>::Random().array() / 2; // [-0.5, 0.5]
        P3d_1.topRows<2>() *= scene_width;
        P3d_1.row(2) = P3d_1.row(2).array() * scene_depth + scene_distance + 0.5f;

        // generate second camera pose
        v3 t12 = baseline * motion_direction;

        v3 centroid = P3d_1.rowwise().mean().transpose(); // translation
        v3 z2 = centroid - t12;
        z2 = z2/z2.norm();
        v3 rotv_12 = v3{-z2[1], z2[0], 0}/(z2[1]*z2[1] + z2[0]*z2[0]) * std::acos(z2[2]); // rotation vector
        transformation G12(to_quaternion(rotation_vector(rotv_12)), t12);
        transformation G21 = invert(G12);

        // generate measurements
        aligned_vector<v2> src;
        aligned_vector<v2> dst;
        for(int i=0; i<num_points; ++i) {
            auto p3d_1 = P3d_1.col(i);
            v3 p3d_2 = G21 * p3d_1;
            src.push_back(p3d_1.head<2>()/p3d_1.z());
            dst.push_back(p3d_2.head<2>()/p3d_2.z());
        }

        std::vector<Essential> Ev;
        estimate_five_point(src, dst, Ev);
        reprojection_errors[test] = Ev.back().error; // get worst reprojection error
    }
    std::nth_element(reprojection_errors.begin(), reprojection_errors.begin() + reprojection_errors.size()/2, reprojection_errors.end());
    EXPECT_NEAR(reprojection_errors[reprojection_errors.size()/2], 0, 8*F_T_EPS);
}

TEST(FivePoint, FiftyPoints)
{
    std::default_random_engine gen(100);
    std::uniform_real_distribution<f_t> r;
    constexpr size_t num_points = 50;
    constexpr size_t num_tests = 100;

    std::vector<f_t> E_errors(num_tests);
    for(int test = 0; test < num_tests; test++) {
        // simulation parameters
        f_t scene_depth = 4*r(gen);
        f_t scene_distance = 1;
        f_t scene_width = 2*r(gen);
        f_t baseline = r(gen) + 0.5f;
        v3 motion_direction{r(gen)-0.5f, r(gen)-0.5f, r(gen)};
        motion_direction /= motion_direction.norm();

        // generate 3D points
        m<3, num_points> P3d_1 = m<3, num_points>::Random().array() / 2; // [-0.5, 0.5]
        P3d_1.topRows<2>() *= scene_width;
        P3d_1.row(2) = P3d_1.row(2).array() * scene_depth + scene_distance + 0.5f;

        // generate second camera pose
        v3 t12 = baseline * motion_direction;
        v3 centroid = P3d_1.rowwise().mean().transpose().cast<f_t>(); // translation
        v3 z2 = centroid - t12;
        z2 = z2/z2.norm();
        v3 rotv_12 = v3{-z2[1], z2[0], 0}/(z2[1]*z2[1] + z2[0]*z2[0]) * std::acos(z2[2]); // rotation vector
        transformation G12(to_quaternion(rotation_vector(rotv_12)), t12);
        transformation G21 = invert(G12);

        // generate measurements
        aligned_vector<v2> src;
        aligned_vector<v2> dst;
        float sigma_noise = 1.f * 1/300.f; // 1 pixel normalized (assumes focal_length = 300 pixels)
        for(int i=0; i<num_points; ++i) {
            const v3& p3d_1 = P3d_1.col(i);
            v3 p3d_2 = G21 * p3d_1;
            v2 noise_src = v2::Random()*sigma_noise;
            v2 noise_dst = v2::Random()*sigma_noise;
            src.push_back(p3d_1.head<2>()/p3d_1.z() + noise_src);
            dst.push_back(p3d_2.head<2>()/p3d_2.z() + noise_dst);
        }

        std::vector<Essential> Ev;
        estimate_five_point(src, dst, Ev);
        m3 Esol = skew(G21.T)*G21.Q.toRotationMatrix();
        E_errors[test] = std::min((Ev[0].E/Ev[0].E.norm()-Esol/Esol.norm()).norm(), (Ev[0].E/Ev[0].E.norm()+Esol/Esol.norm()).norm());
    }
    std::nth_element(E_errors.begin(), E_errors.begin() + E_errors.size()/2, E_errors.end());
    EXPECT_NEAR(E_errors[E_errors.size()/2], 0, 1e-2);
}

TEST(FivePoint, FivePointRansac) // TODO: This test is slow, review in the future
{
    std::minstd_rand gen(100);
    std::uniform_real_distribution<f_t> r;
    constexpr size_t num_points = 50;
    constexpr size_t num_outliers = 10;
    constexpr size_t num_tests = 100;
    constexpr size_t max_iterations = 30;

    std::vector<f_t> E_errors(num_tests);
    for(int test = 0; test < num_tests; test++) {
        // simulation parameters
        f_t scene_depth = 4*r(gen);
        f_t scene_distance = 1;
        f_t scene_width = 2*r(gen);
        f_t baseline = r(gen) + 0.5f;
        v3 motion_direction{r(gen)-0.5f, r(gen)-0.5f, r(gen)};
        motion_direction /= motion_direction.norm();

        // generate 3D points
        m<3, num_points> P3d_1 = m<3, num_points>::Random().array() / 2; // [-0.5, 0.5]
        P3d_1.topRows<2>() *= scene_width;
        P3d_1.row(2) = P3d_1.row(2).array() * scene_depth + scene_distance + 0.5f;

        // generate second camera pose
        v3 t12 = baseline * motion_direction;
        v3 centroid = P3d_1.rowwise().mean().transpose().cast<f_t>(); // translation
        v3 z2 = centroid - t12;
        z2 = z2/z2.norm();
        v3 rotv_12 = v3{-z2[1], z2[0], 0}/(z2[1]*z2[1] + z2[0]*z2[0]) * std::acos(z2[2]); // rotation vector
        transformation G12(to_quaternion(rotation_vector(rotv_12)), t12);
        transformation G21 = invert(G12);

        // generate measurements
        aligned_vector<v2> src;
        aligned_vector<v2> dst;
        f_t sigma_noise = 1.f * 1/300.f; // 1 pixel normalized (assumes focal_length = 300 pixels)
        for(int i=0; i<num_points; ++i) {
            const v3& p3d_1 = P3d_1.col(i);
            v3 p3d_2 = G21 * p3d_1;
            v2 noise_src = v2::Random()*sigma_noise;
            v2 noise_dst = v2::Random()*sigma_noise;
            src.push_back(p3d_1.head<2>()/p3d_1.z() + noise_src);
            dst.push_back(p3d_2.head<2>()/p3d_2.z() + noise_dst);
        }
        {std::default_random_engine gen(100); std::shuffle(src.end()-num_outliers, src.end(), gen);}

        m3 E;
        estimate_five_point(src, dst, E, gen, max_iterations, 3*sigma_noise);
        m3 Esol = skew(G21.T)*G21.Q.toRotationMatrix();
        E_errors[test] = std::min((E/E.norm()-Esol/Esol.norm()).norm(), (E/E.norm()+Esol/Esol.norm()).norm());
    }
    std::nth_element(E_errors.begin(), E_errors.begin() + E_errors.size()/2, E_errors.end());
    EXPECT_NEAR(E_errors[E_errors.size()/2], 0, 1e-1);
 }


