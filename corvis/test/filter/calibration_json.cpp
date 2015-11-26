#include "gtest/gtest.h"
#include "calibration_json.h"
#include "calibration_defaults.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

TEST(calibration_json, SerializeDeserialize)
{
    device_parameters cal, calDeserialized, def = {};
    EXPECT_TRUE(calibration_load_defaults(DEVICE_TYPE_UNKNOWN, cal));

    try
    {
        std::string jsonString;
        EXPECT_TRUE(calibration_serialize(cal, jsonString));
        EXPECT_GT(jsonString.length(), 0); // expect non-zero length

        EXPECT_TRUE(calibration_deserialize(jsonString, calDeserialized, &def));
    }
    catch (runtime_error)
    {
        FAIL();
    }

    // just do some spot checking
    EXPECT_EQ(cal.calibrationVersion, calDeserialized.calibrationVersion);
    EXPECT_FLOAT_EQ(cal.Fx, calDeserialized.Fx);
    EXPECT_FLOAT_EQ(cal.Cx, calDeserialized.Cx);
}

TEST(calibration_json, DeserializeCalibration)
{
    device_parameters calDeserialized, def = {};

    try
    {
        std::string jsonString;
        EXPECT_FALSE(calibration_deserialize(jsonString, calDeserialized, &def));
    }
    catch (...)
    {
        FAIL();
    }
}
