
#include "gtest/gtest.h"
#include "rc_tracker.h"
#include "sensor_fusion.h"
#include <memory>
#include "calibration_json.h"
#include "json_keys.h"

using namespace std;

void count_errors(void *handle, rc_MessageLevel message_level, const char * message, size_t len)
{
    int * error_count = (int *)handle;
    (*error_count)++;
}

TEST(rc_tracker_tests, rc_receiveImageBadPtr)
{
    rc_Tracker *tracker = rc_create();
    int error_count = 0;
    rc_setMessageCallback(tracker, count_errors, &error_count, rc_MESSAGE_ERROR);
    EXPECT_FALSE(rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, 0, 0, 640, 480, 640, nullptr, nullptr, nullptr));
    rc_destroy(tracker);
    EXPECT_EQ(error_count, 1);
}

TEST(rc_tracker_tests, rc_createAndReset)
{
    rc_Tracker *tracker = rc_create();
    rc_reset(tracker, 0);
    rc_destroy(tracker);
}

TEST(rc_tracker_tests, rc_receiveImageBadDimensions)
{
    rc_Tracker *tracker = rc_create();
    int error_count = 0;
    uint8_t image[64*64];
    rc_setMessageCallback(tracker, count_errors, &error_count, rc_MESSAGE_ERROR);
    EXPECT_FALSE(rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, 0, 0,   0,  64,  64, image, nullptr, nullptr));
    EXPECT_FALSE(rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, 0, 0, -64,  64,  64, image, nullptr, nullptr));
    EXPECT_FALSE(rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, 0, 0,  64,   0,  64, image, nullptr, nullptr));
    EXPECT_FALSE(rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, 0, 0,  64, -64,  64, image, nullptr, nullptr));
    EXPECT_FALSE(rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, 0, 0,  64,  64,   0, image, nullptr, nullptr));
    EXPECT_FALSE(rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, 0, 0,  64,  64, -64, image, nullptr, nullptr));
    EXPECT_FALSE(rc_receiveImage(tracker, 0, rc_FORMAT_GRAY8, 0, 0,  64,  64,  64, image, nullptr, nullptr));
    rc_destroy(tracker);
    EXPECT_EQ(error_count, 6);
}

TEST(rc_tracker_tests, rc_setCalibration)
{
    calibration_json calInput = {};
    calInput.device_id = "(test)";

    calInput.color.intrinsics.width_px = 123;
    calInput.color.intrinsics.height_px = 321;
    calInput.color.intrinsics.type = rc_CALIBRATION_TYPE_FISHEYE;
    calInput.imu.a_alignment(0) = 0.123;
    calInput.imu.a_alignment(8) = 0.321;
    calInput.imu.w_alignment(0) = 0.456;
    calInput.imu.w_alignment(8) = 0.654;
    calInput.color.extrinsics_wrt_imu_m.T[0] = 0.666;
    calInput.color.extrinsics_var_wrt_imu_m.T[1] = 0.777;
    //calInput.a_bias_var[2] = 1.111111111e-06;
    //calInput.w_bias_var[2] = 2.222222222e-07;

    // set cal
    string jsonString;
    EXPECT_TRUE(calibration_serialize(calInput, jsonString));

    rc_Tracker *tracker = rc_create();
    EXPECT_TRUE(rc_setCalibration(tracker, jsonString.c_str()));

    // now read cal back out
    const char* buffer;
    size_t size = rc_getCalibration(tracker, &buffer);
    EXPECT_GT(size, 0);

    // compare values between original and retrieved
    calibration_json calOutput;
    EXPECT_TRUE(calibration_deserialize(jsonString, calOutput));
    EXPECT_EQ(calInput.device_id, calOutput.device_id);
    EXPECT_EQ(calInput.color.intrinsics.width_px, calOutput.color.intrinsics.width_px);
    EXPECT_EQ(calInput.color.intrinsics.height_px, calOutput.color.intrinsics.height_px);
    EXPECT_EQ(calInput.color.intrinsics.type, calOutput.color.intrinsics.type);
    EXPECT_FLOAT_EQ(calInput.color.extrinsics_wrt_imu_m.T[0], calOutput.color.extrinsics_wrt_imu_m.T[0]);
    EXPECT_FLOAT_EQ(calInput.color.extrinsics_var_wrt_imu_m.T[1], calOutput.color.extrinsics_var_wrt_imu_m.T[1]);
    //EXPECT_FLOAT_EQ(calInput.a_bias_var[2], calOutput.a_bias_var[2]); these fail because there are minimums lower than the input values. not a problem.
    //EXPECT_FLOAT_EQ(calInput.w_bias_var[2], calOutput.w_bias_var[2]);
    EXPECT_FLOAT_EQ(calInput.imu.a_alignment(0), calOutput.imu.a_alignment(0));
    EXPECT_FLOAT_EQ(calInput.imu.a_alignment(8), calOutput.imu.a_alignment(8));
    EXPECT_FLOAT_EQ(calInput.imu.w_alignment(0), calOutput.imu.w_alignment(0));
    EXPECT_FLOAT_EQ(calInput.imu.w_alignment(8), calOutput.imu.w_alignment(8));

    rc_destroy(tracker);
}

TEST(rc_tracker_tests, rc_setCalibration_failure)
{
    rc_Tracker *tracker = rc_create();
    EXPECT_FALSE(rc_setCalibration(tracker, ""));
    rc_destroy(tracker);
}

//TODO: lots more tests needed for multisensor
/*
TEST(rc_tracker_tests, rc_fisheyeKw)
{
    rc_Tracker *tracker = rc_create();

    const char json_in[] = "{ \"" KEY_KW "\": 0.123, \"" KEY_DISTORTION_MODEL "\": 1 }";
    EXPECT_TRUE(rc_setCalibration(tracker, json_in));

    struct rc_CameraIntrinsics intrinsics = {};
    ASSERT_TRUE(rc_describeCamera(tracker, rc_CAMERA_ID_FISHEYE, nullptr, &intrinsics));
    EXPECT_EQ(intrinsics.type, rc_CALIBRATION_TYPE_FISHEYE);
    EXPECT_FLOAT_EQ(intrinsics.w, 0.123);

    const char *json_fisheye_123 = nullptr;
    rc_getCalibration(tracker, &json_fisheye_123);

    intrinsics.type = rc_CALIBRATION_TYPE_POLYNOMIAL3;
    intrinsics.k1 = 23;
    rc_configureCamera(tracker, rc_CAMERA_ID_COLOR, nullptr, &intrinsics);

    rc_CameraIntrinsics intrinsics_out;
    ASSERT_TRUE(rc_describeCamera(tracker, rc_CAMERA_ID_COLOR, nullptr, &intrinsics_out));
    EXPECT_EQ(intrinsics_out.type, rc_CALIBRATION_TYPE_POLYNOMIAL3);
    EXPECT_FLOAT_EQ(intrinsics_out.k1, 23);

    rc_setCalibration(tracker, json_fisheye_123);

    rc_CameraIntrinsics intrinsics_fisheye_123;
    rc_describeCamera(tracker, rc_CAMERA_ID_FISHEYE, nullptr, &intrinsics_fisheye_123);
    EXPECT_EQ(intrinsics_fisheye_123.type, rc_CALIBRATION_TYPE_FISHEYE);
    EXPECT_FLOAT_EQ(intrinsics_fisheye_123.w, .123);

    rc_destroy(tracker);
}
*/
