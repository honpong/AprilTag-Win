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

TEST(calibration_json, calibration_deserialize)
{
    rcCalibration defaults;

    snprintf(defaults.deviceName, sizeof(defaults.deviceName), "%s", R"(test)");
    defaults.image_width = 456;
    defaults.image_height = 321;
    defaults.shutterDelay = 0.789;
    defaults.accelerometerTransform[7] = 0.666; // this should not be set, since JSON contains a value for it
    defaults.accelerometerTransform[8] = 0.777; // this should be set, since last element is missing in JSON

    const char json[] = "{ \"" KEY_IMAGE_WIDTH "\" : 123, \"" KEY_ACCEL_TRANSFORM "\" : [0,0,0,0,0,0,0,0.6] }"; // note that last element of array is missing

    rcCalibration calOutput;
    EXPECT_TRUE(calibration_deserialize(json, calOutput, &defaults));

    // check that default value was NOT used
    EXPECT_EQ(123, calOutput.image_width);
    EXPECT_FLOAT_EQ(calOutput.accelerometerTransform[7], 0.6);

    // check that default values were set
    EXPECT_STREQ("test", calOutput.deviceName);
    EXPECT_EQ(defaults.image_height, calOutput.image_height);
    EXPECT_EQ(defaults.shutterDelay, calOutput.shutterDelay);
    EXPECT_FLOAT_EQ(calOutput.accelerometerTransform[8], defaults.accelerometerTransform[8]);
}
