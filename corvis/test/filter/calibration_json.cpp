#include "gtest/gtest.h"
#include "calibration_json.h"
#include "calibration_defaults.h"
#include "json_keys.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

TEST(calibration_json, SerializeDeserialize)
{
    calibration_json cal, calDeserialized;
    EXPECT_TRUE(calibration_load_defaults(DEVICE_TYPE_UNKNOWN, cal));

    try
    {
        std::string jsonString;
        EXPECT_TRUE(calibration_serialize(cal, jsonString));
        EXPECT_GT(jsonString.length(), 0); // expect non-zero length

        EXPECT_TRUE(calibration_deserialize(jsonString, calDeserialized));
    }
    catch (runtime_error)
    {
        FAIL();
    }

    // just do some spot checking
    EXPECT_EQ(cal.version, calDeserialized.version);
    EXPECT_FLOAT_EQ(cal.color.intrinsics.f_x_px, calDeserialized.color.intrinsics.f_x_px);
    EXPECT_FLOAT_EQ(cal.color.intrinsics.c_x_px, calDeserialized.color.intrinsics.c_x_px);
}

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

TEST(calibration_json, get_parameters_for_device)
{
    calibration_json cal;
    get_parameters_for_device(DEVICE_TYPE_GIGABYTES11, &cal);
    EXPECT_EQ(cal.version, CALIBRATION_VERSION_LEGACY);
}

TEST(calibration_json, device_set_resolution)
{
    calibration_json cal;
    device_set_resolution(&cal, 96, 69);

    EXPECT_EQ(96, cal.color.intrinsics.width_px);
    EXPECT_EQ(69, cal.color.intrinsics.height_px);
}

