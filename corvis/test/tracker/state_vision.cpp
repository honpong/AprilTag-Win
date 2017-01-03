#include "gtest/gtest.h"
#include "state_vision.h"
#include <random>
#include <limits>

namespace rc { namespace testing {

struct StateVision : public ::testing::Test {
    state_camera camera;
    std::default_random_engine gen;
    std::uniform_real_distribution<float> normalized_coord;
    StateVision() : normalized_coord(-1.1, 1.1) {}
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
};

TEST_F(StateVision, UndistortPolynomialIdentity)
{
    camera.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
    camera.intrinsics.k1.v = -0.00959507;
    camera.intrinsics.k2.v = 0.008005806;
    camera.intrinsics.k3.v = 0.000281827;

    for (int i=0; i<1000; i++) {
        feature_t p = { normalized_coord(gen), normalized_coord(gen) },
                  distort_undistort_p = camera.intrinsics.distort_feature(camera.intrinsics.undistort_feature(p));
        EXPECT_NEAR(p.x(), distort_undistort_p.x(), 2*std::numeric_limits<float>::epsilon());
        EXPECT_NEAR(p.y(), distort_undistort_p.y(), 2*std::numeric_limits<float>::epsilon());
    }
}

TEST_F(StateVision, UndistortPolynomialZero)
{
    camera.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
    camera.intrinsics.k1.v = -0.00959507;
    camera.intrinsics.k2.v = 0.008005806;
    camera.intrinsics.k3.v = 0.000281827;

    feature_t p = {0, 0};
    feature_t undistort_p = camera.intrinsics.undistort_feature(p);
    EXPECT_NEAR(undistort_p.x(), 0., std::numeric_limits<f_t>::epsilon());
    EXPECT_NEAR(undistort_p.y(), 0., std::numeric_limits<f_t>::epsilon());
}

TEST_F(StateVision, DistortPolynomialZero)
{
    camera.intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
    camera.intrinsics.k1.v = -0.00959507;
    camera.intrinsics.k2.v = 0.008005806;
    camera.intrinsics.k3.v = 0.000281827;

    feature_t p = {0, 0};
    feature_t distort_p = camera.intrinsics.distort_feature(p);
    EXPECT_NEAR(distort_p.x(), 0., std::numeric_limits<f_t>::epsilon());
    EXPECT_NEAR(distort_p.y(), 0., std::numeric_limits<f_t>::epsilon());
}

TEST_F(StateVision, UndistortFisheyeIdentity)
{
    camera.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
    camera.intrinsics.k1.v = 0.95;

    for (int i=0; i<1000; i++) {
        feature_t p = { normalized_coord(gen), normalized_coord(gen) },
                  distort_undistort_p = camera.intrinsics.distort_feature(camera.intrinsics.undistort_feature(p));
        EXPECT_NEAR(p.x(), distort_undistort_p.x(), 2*std::numeric_limits<f_t>::epsilon());
        EXPECT_NEAR(p.y(), distort_undistort_p.y(), 2*std::numeric_limits<f_t>::epsilon());
    }

}

TEST_F(StateVision, UndistortFisheyeZero)
{
    camera.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
    camera.intrinsics.k1.v = 0.95;

    feature_t p = {0, 0};
    feature_t undistort_p = camera.intrinsics.undistort_feature(p);
    EXPECT_NEAR(undistort_p.x(), 0., std::numeric_limits<f_t>::epsilon());
    EXPECT_NEAR(undistort_p.y(), 0., std::numeric_limits<f_t>::epsilon());
}

TEST_F(StateVision, DistortFisheyeZero)
{
    camera.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
    camera.intrinsics.k1.v = 0.95;

    feature_t p = {0, 0};
    feature_t distort_p = camera.intrinsics.distort_feature(p);
    EXPECT_NEAR(distort_p.x(), 0., std::numeric_limits<f_t>::epsilon());
    EXPECT_NEAR(distort_p.y(), 0., std::numeric_limits<f_t>::epsilon());
}

TEST_F(StateVision, NormalizeIdentity)
{
    camera.intrinsics.image_width = 640;
    camera.intrinsics.image_height = 480;
    camera.intrinsics.focal_length.v = 400.f/camera.intrinsics.image_height;
    camera.intrinsics.center_x.v = (320 - camera.intrinsics.image_width / 2. + .5) / camera.intrinsics.image_height;
    camera.intrinsics.center_y.v = (240 - camera.intrinsics.image_width / 2. + .5) / camera.intrinsics.image_height;

    for (int i=0; i<1000; i++) {
        feature_t p = { normalized_coord(gen), normalized_coord(gen) };
        feature_t normal_unnormal_p = camera.intrinsics.normalize_feature(camera.intrinsics.unnormalize_feature(p));
        EXPECT_NEAR(p.x(), normal_unnormal_p.x(), 2*std::numeric_limits<f_t>::epsilon());
        EXPECT_NEAR(p.y(), normal_unnormal_p.y(), 2*std::numeric_limits<f_t>::epsilon());
    }
}

} /*testing*/ } /*rc*/
