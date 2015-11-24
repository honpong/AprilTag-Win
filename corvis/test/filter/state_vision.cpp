#include "gtest/gtest.h"
#include "state_vision.h"
#include <random>
#include <limits>

namespace rc { namespace testing {

struct StateVision : public ::testing::Test {
    covariance c;
    state_vision state;
    std::default_random_engine gen;
    std::uniform_real_distribution<float> normalized_coord;
    StateVision() : state(c), normalized_coord(-1.1, 1.1) {}
};

TEST_F(StateVision, UndistortPolynomialIdentity)
{
    state.fisheye = false;
    state.k1.v = -0.00959507;
    state.k2.v = 0.008005806;
    state.k3.v = 0.000281827;

    for (int i=0; i<1000; i++) {
        feature_t p = { normalized_coord(gen), normalized_coord(gen) },
                  distort_undistort_p = state.distort_feature(state.undistort_feature(p));
        EXPECT_FLOAT_EQ(p.x, distort_undistort_p.x);
        EXPECT_FLOAT_EQ(p.y, distort_undistort_p.y);
    }
}

TEST_F(StateVision, UndistortFisheyeIdentity)
{
    state.fisheye = true;
    state.k1.v = 0.95;

    for (int i=0; i<1000; i++) {
        feature_t p = { normalized_coord(gen), normalized_coord(gen) },
                  distort_undistort_p = state.distort_feature(state.undistort_feature(p));
        EXPECT_FLOAT_EQ(p.x, distort_undistort_p.x);
        EXPECT_FLOAT_EQ(p.y, distort_undistort_p.y);
    }

}

TEST_F(StateVision, NormalizeIdentity)
{
    state.image_width = 640;
    state.image_height = 480;
    state.focal_length.v = 400.f/state.image_height;
    state.center_x.v = (320 - state.image_width / 2. + .5) / state.image_height;
    state.center_y.v = (240 - state.image_width / 2. + .5) / state.image_height;

    for (int i=0; i<1000; i++) {
        feature_t p = { normalized_coord(gen), normalized_coord(gen) };
        feature_t normal_unnormal_p = state.normalize_feature(state.unnormalize_feature(p));
        EXPECT_NEAR(p.x, normal_unnormal_p.x, std::numeric_limits<float>::epsilon());
        EXPECT_NEAR(p.y, normal_unnormal_p.y, std::numeric_limits<float>::epsilon());
    }
}

} /*testing*/ } /*rc*/
