#include "gtest/gtest.h"
#include "calibration_json_store.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

bool nearlyEqual(float a, float b, float epsilon) 
{
    const float absA = fabs(a);
    const float absB = fabs(b);
    const float diff = fabs(a - b);

    if (a == b)  // shortcut, handles infinities
    {
        return true;
    }
    else if (a == 0 || b == 0 || diff < FLT_MIN) 
    {
        // a or b is zero or both are extremely close to it
        // relative error is less meaningful here
        return diff < (epsilon * FLT_MIN);
    }
    else  // use relative error
    {
        return diff / fmin((absA + absB), FLT_MAX) < epsilon;
    }
}

TEST(calibration_json_store_tests, SerializeDeserialize)
{
    corvis_device_parameters cal, calDeserialized;
    EXPECT_TRUE(calibration_load_defaults(DEVICE_TYPE_UNKNOWN, cal));

    try
    {
        std::string jsonString;
        EXPECT_TRUE(calibration_serialize(cal, jsonString));
        EXPECT_TRUE(jsonString.length()); // expect non-zero length

        EXPECT_TRUE(calibration_deserialize(jsonString, calDeserialized));
    }
    catch (runtime_error)
    {
        FAIL();
    }

    // just do some spot checking
    EXPECT_EQ(cal.version, calDeserialized.version);
    EXPECT_TRUE(nearlyEqual(cal.Fx, calDeserialized.Fx, FLT_EPSILON));
    EXPECT_TRUE(nearlyEqual(cal.Cx, calDeserialized.Cx, FLT_EPSILON));
}

TEST(calibration_json_store_tests, DeserializeCalibration)
{
    corvis_device_parameters calDeserialized;

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
