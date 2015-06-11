#include "gtest/gtest.h"
#include "calibration_json_store.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace RealityCap;
using namespace std;

const float testFloat = .1;

TEST(calibration_json_store_tests, NewDelete)
{
    calibration_json_store* instance = new calibration_json_store();
    delete instance;
}

fpos_t filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

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

TEST(calibration_json_store_tests, SaveCalibration)
{
    remove(CALIBRATION_FILE_NAME);

    calibration_json_store calStore;
    corvis_device_parameters params;
    params.Fx = testFloat;

    try
    {
        calStore.SaveCalibration(params);
    }
    catch (runtime_error)
    {
        FAIL();
    }

    // check that file size is greater than zero
    EXPECT_GT(filesize(CALIBRATION_FILE_NAME), 0);
}

// there will be an existing calibration file because of above test
TEST(calibration_json_store_tests, LoadCalibration)
{
    calibration_json_store calStore;
    try
    {
        corvis_device_parameters cal;
        calStore.LoadCalibration(cal);
        EXPECT_TRUE(nearlyEqual(testFloat, cal.Fx, FLT_EPSILON));
    }
    catch (runtime_error)
    {
        FAIL();
    }
}

TEST(calibration_json_store_tests, HasCalibration)
{
    calibration_json_store calStore;
    corvis_device_parameters cal;

    try
    {
        // save the defaults as a valid calibration. next test cleans up.
        calStore.LoadCalibrationDefaults(DEVICE_TYPE_UNKNOWN, cal);
        calStore.SaveCalibration(cal);
        EXPECT_TRUE(calStore.HasCalibration());
    }
    catch (runtime_error)
    {
        FAIL();
    }
}

TEST(calibration_json_store_tests, HasCalibration_MissingFile)
{
    calibration_json_store calStore;
    try
    {
        calStore.ClearCalibration();
        EXPECT_FALSE(calStore.HasCalibration());
    }
    catch (runtime_error)
    {
        FAIL();
    }
}

TEST(calibration_json_store_tests, SerializeDeserialize)
{
    corvis_device_parameters cal, calDeserialized;
    calibration_json_store calStore;
    calStore.SetCalibrationLocation("calibration/");
    EXPECT_TRUE(calStore.LoadCalibrationDefaults(DEVICE_TYPE_UNKNOWN, cal));

    try
    {
        std::string jsonString;
        EXPECT_TRUE(calibration_json_store::SerializeCalibration(cal, jsonString));
        EXPECT_TRUE(jsonString.length()); // expect non-zero length

        EXPECT_TRUE(calibration_json_store::DeserializeCalibration(jsonString, calDeserialized));
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
        EXPECT_FALSE(calibration_json_store::DeserializeCalibration(jsonString, calDeserialized));
    }
    catch (...)
    {
        FAIL();
    }
}
