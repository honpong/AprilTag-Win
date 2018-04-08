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

TEST(Camera, UndistortImagePointNoDistortion) {
    for(int x = 0; x < width; x++)
        for(int y = 0; y < height; y++)
            TEST_V2_NEAR(v2{x,y}, nodistortion.undistort_image_point(x, y), F_T_EPS);
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

            EXPECT_V2_NEAR(expected, point, F_T_EPS);
        }
}

TEST(Camera, CalibrateImagePointNoDistortion) {
    for(int x = 0; x < width; x++)
        for(int y = 0; y < height; y++); {
            v3 point = nodistortion.calibrate_image_point(x, y);
            v3 expected = v3{(x - center_x)/focal_length, (y - center_y)/focal_length, 1};
            EXPECT_V3_NEAR(expected, point, F_T_EPS);
        }
}

TEST(Camera, CalibrateImagePointTypicalDistortion) {
    for(int x = 0; x < width; x++)
        for(int y = 0; y < height; y++); {
            v3 point = typicalcamera.calibrate_image_point(x, y);

            f_t xp = (x - typical_center_x)/focal_length;
            f_t yp = (y - typical_center_y)/focal_length;
            f_t r2 = xp*xp + yp*yp; 
            f_t kr = 1 + typical_k1 * r2 +  typical_k2 * r2*r2 + typical_k3 * r2*r2*r2;
            v3 expected = {xp/kr, yp/kr, 1};
            EXPECT_V3_NEAR(expected, point, F_T_EPS);
        }
}

TEST(Camera, ProjectImagePointNoDistortion) {
    for(int x = 0; x < width; x++)
        for(int y = 0; y < height; y++); {
            v3 point = nodistortion.project_image_point(x, y);
            v3 expected = v3{(x - center_x)/focal_length, (y - center_y)/focal_length, 1};
            EXPECT_V3_NEAR(expected, point, F_T_EPS);
        }
}

TEST(Camera, ProjectImagePointTypicalDistortion) {
    // The projection should be unaffected by distortion parameters
    for(int x = 0; x < width; x++)
        for(int y = 0; y < height; y++); {
            v3 point = typicalcamera.project_image_point(x, y);
            v3 expected = {(x - typical_center_x)/focal_length, (y - typical_center_y)/focal_length, 1};
            camera_test_v4_equal(expected, point);
        }
}
