#include "gtest/gtest.h"
#include "calibration_json.h"
#include "json_keys.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

TEST(calibration_json, DeserializeCalibration)
{
    calibration_json calDeserialized;

    try
    {
        std::string jsonString;
        EXPECT_FALSE(calibration_deserialize(jsonString, calDeserialized));
    }
    catch (...)
    {
        FAIL();
    }
}

TEST(calibration_json, calibration_deserialize)
{
    const char json[] = "{ \"" KEY_DEVICE_NAME "\" : \"test\", \"" KEY_IMAGE_WIDTH "\" : 123, \"" KEY_ACCEL_TRANSFORM "\" : [0,0,0, 0,0,0, 0,0.6,0] }"; // note that last element of array is missing

    calibration_json calOutput;
    EXPECT_TRUE(calibration_deserialize(json, calOutput));

    EXPECT_EQ(123, calOutput.color.intrinsics.width_px);
    EXPECT_FLOAT_EQ(calOutput.imu.a_alignment(7), 0.6);
    EXPECT_EQ("test", calOutput.device_id);
}
