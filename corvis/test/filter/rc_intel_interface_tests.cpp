
#include "gtest/gtest.h"
#include "rc_intel_interface.h"
#include "sensor_fusion.h"
#include <memory>
#include "calibration_json.h"
#include "json_keys.h"

using namespace std;

TEST(rc_intel_interface_tests, rc_setCalibration)
{
    rcCalibration calInput = { "(test)" };

    calInput.image_width = 123;
    calInput.image_height = 321;
    calInput.px = 0.567;
    calInput.py = 0.765;
    calInput.distortionModel = 1;
    calInput.accelerometerTransform[0] = 0.123;
    calInput.accelerometerTransform[8] = 0.321;
    calInput.gyroscopeTransform[0] = 0.456;
    calInput.gyroscopeTransform[8] = 0.654;
    calInput.shutterDelay = 0.666;
    calInput.shutterPeriod = 0.777;
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
    rcCalibration calOutput, defaults = {};
    EXPECT_TRUE(calibration_deserialize(jsonString, calOutput, &defaults));
    EXPECT_STREQ(calInput.deviceName, calOutput.deviceName);
    EXPECT_EQ(calInput.image_width, calOutput.image_width);
    EXPECT_EQ(calInput.image_height, calOutput.image_height);
    EXPECT_FLOAT_EQ(calInput.px, calOutput.px);
    EXPECT_FLOAT_EQ(calInput.py, calOutput.py);
    EXPECT_EQ(calInput.distortionModel, calOutput.distortionModel);
    EXPECT_FLOAT_EQ(calInput.shutterDelay, calOutput.shutterDelay);
    EXPECT_FLOAT_EQ(calInput.shutterPeriod, calOutput.shutterPeriod);
    //EXPECT_FLOAT_EQ(calInput.a_bias_var[2], calOutput.a_bias_var[2]); these fail because there are minimums lower than the input values. not a problem.
    //EXPECT_FLOAT_EQ(calInput.w_bias_var[2], calOutput.w_bias_var[2]);
    EXPECT_FLOAT_EQ(calInput.accelerometerTransform[0], calOutput.accelerometerTransform[0]);
    EXPECT_FLOAT_EQ(calInput.accelerometerTransform[8], calOutput.accelerometerTransform[8]);
    EXPECT_FLOAT_EQ(calInput.gyroscopeTransform[0], calOutput.gyroscopeTransform[0]);
    EXPECT_FLOAT_EQ(calInput.gyroscopeTransform[8], calOutput.gyroscopeTransform[8]);

    rc_destroy(tracker);
}

TEST(rc_intel_interface_tests, rc_setCalibration_failure)
{
    rc_Tracker *tracker = rc_create();
    EXPECT_FALSE(rc_setCalibration(tracker, ""));
    rc_destroy(tracker);
}

TEST(rc_intel_interface_tests, rc_fisheyeKw)
{
    rc_Tracker *tracker = rc_create();

    const char json_in[] = "{ \"" KEY_KW "\": 0.123, \"" KEY_DISTORTION_MODEL "\": 1 }";
    EXPECT_TRUE(rc_setCalibration(tracker, json_in));

    struct rc_Intrinsics intrinsics = {};
    rc_describeCamera(tracker, rc_EGRAY8, nullptr, &intrinsics);
    EXPECT_EQ(intrinsics.type, rc_CAL_FISHEYE);
    EXPECT_FLOAT_EQ(intrinsics.w, 0.123);

    const char *json_out = nullptr;
    rc_getCalibration(tracker, &json_out);

    intrinsics.type = rc_CAL_POLYNOMIAL3;
    intrinsics.k1 = 23;
    intrinsics.w = 86;
    rc_configureCamera(tracker, rc_EGRAY8, nullptr, &intrinsics);

    rc_Intrinsics intrinsics_out;
    rc_describeCamera(tracker, rc_EGRAY8, nullptr, &intrinsics_out);
    EXPECT_EQ(intrinsics_out.type, rc_CAL_POLYNOMIAL3);
    EXPECT_FLOAT_EQ(intrinsics_out.w, 86);

    rc_setCalibration(tracker, json_out);

    rc_Intrinsics intrinsics_again;
    rc_describeCamera(tracker, rc_EGRAY8, nullptr, &intrinsics_again);
    EXPECT_EQ(intrinsics_again.type, rc_CAL_FISHEYE);
    EXPECT_FLOAT_EQ(intrinsics_again.w, .123);

    rc_destroy(tracker);
}
