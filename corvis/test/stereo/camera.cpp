#include "gtest/gtest.h"
#include "camera.h"

const int width = 640;
const int height = 480;
const int bwchannels = 1;
const int center_x = 320;
const int center_y = 240;
const float focal_length = 500;
const float typical_center_x = 125.3;
const float typical_center_y = 221.5;
const float typical_k1 = 0.075;
const float typical_k2 = -0.013;
const float typical_k3 = 0;
const camera nodistortion(width, height, bwchannels, center_x, center_y, 0, 0, 0, focal_length);
const camera typicalcamera(width, height, bwchannels, typical_center_x, typical_center_y, typical_k1, typical_k2, typical_k3, focal_length);

void camera_test_v4_equal(const v4 & p1, const v4 & p2)
{
    EXPECT_FLOAT_EQ(p1[0], p2[0]);
    EXPECT_FLOAT_EQ(p1[1], p2[1]);
    EXPECT_FLOAT_EQ(p1[2], p2[2]);
    EXPECT_FLOAT_EQ(p1[3], p2[3]);
}

void camera_test_feature_t_equal(const feature_t & p1, const feature_t & p2)
{
    EXPECT_FLOAT_EQ(p1.x, p2.x);
    EXPECT_FLOAT_EQ(p1.y, p2.y);
}

TEST(Camera, UndistortImagePointNoDistortion) {
    int x, y;

    for(x = 0; x < width; x++)
        for(y = 0; y < height; y++); {
            feature_t point = nodistortion.undistort_image_point(x, y);
            feature_t expected = {.x = x, .y = y};
            camera_test_feature_t_equal(expected, point);
        }
}

TEST(Camera, UndistortImagePointTypicalDistortion) {
    int x, y;

    for(x = 0; x < width; x++)
        for(y = 0; y < height; y++); {
            feature_t point = typicalcamera.undistort_image_point(x, y);

            feature_t expected;
            f_t xp = (x - typical_center_x)/focal_length;
            f_t yp = (y - typical_center_y)/focal_length;
            f_t r2 = xp*xp + yp*yp; 
            f_t kr = 1 + typical_k1 * r2 +  typical_k2 * r2*r2 + typical_k3 * r2*r2*r2;
            // focal length cancels out
            expected.x = (x - typical_center_x)/kr + typical_center_x;
            expected.y = (y - typical_center_y)/kr + typical_center_y;

            camera_test_feature_t_equal(expected, point);
        }
}

TEST(Camera, CalibrateImagePointNoDistortion) {
    int x, y;

    for(x = 0; x < width; x++)
        for(y = 0; y < height; y++); {
            v4 point = nodistortion.calibrate_image_point(x, y);
            v4 expected = v4((x - center_x)/focal_length, (y - center_y)/focal_length, 1, 1);
            camera_test_v4_equal(expected, point);
        }
}

TEST(Camera, CalibrateImagePointTypicalDistortion) {
    int x, y;

    for(x = 0; x < width; x++)
        for(y = 0; y < height; y++); {
            v4 point = typicalcamera.calibrate_image_point(x, y);

            f_t xp = (x - typical_center_x)/focal_length;
            f_t yp = (y - typical_center_y)/focal_length;
            f_t r2 = xp*xp + yp*yp; 
            f_t kr = 1 + typical_k1 * r2 +  typical_k2 * r2*r2 + typical_k3 * r2*r2*r2;
            v4 expected = v4(xp/kr, yp/kr, 1, 1);

            camera_test_v4_equal(expected, point);
        }
}

TEST(Camera, ProjectImagePointNoDistortion) {
    int x, y;

    for(x = 0; x < width; x++)
        for(y = 0; y < height; y++); {
            v4 point = nodistortion.project_image_point(x, y);
            v4 expected = v4((x - center_x)/focal_length, (y - center_y)/focal_length, 1, 1);
            camera_test_v4_equal(expected, point);
        }
}

TEST(Camera, ProjectImagePointTypicalDistortion) {
    // The projection should be unaffected by distortion parameters
    int x, y;

    for(x = 0; x < width; x++)
        for(y = 0; y < height; y++); {
            v4 point = typicalcamera.project_image_point(x, y);
            v4 expected = v4((x - typical_center_x)/focal_length, (y - typical_center_y)/focal_length, 1, 1);
            camera_test_v4_equal(expected, point);
        }
}
