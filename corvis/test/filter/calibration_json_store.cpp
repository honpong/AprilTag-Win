#include "gtest/gtest.h"
#include "calibration_json_store.h"
#include "calibration_defaults.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

TEST(calibration_json_store_tests, SerializeDeserialize)
{
    device_parameters cal, calDeserialized;
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
    EXPECT_EQ(cal.calibrationVersion, calDeserialized.calibrationVersion);
    EXPECT_FLOAT_EQ(cal.Fx, calDeserialized.Fx);
    EXPECT_FLOAT_EQ(cal.Cx, calDeserialized.Cx);
}

TEST(calibration_json_store_tests, DeserializeCalibration)
{
    device_parameters calDeserialized;

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
