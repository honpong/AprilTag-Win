#include "gtest/gtest.h"
#include "calibration_json_store.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace RealityCap;
using namespace std;

const float testFloat = .1;

TEST(calibration_data_store_tests, NewDelete)
{
    calibration_json_store* instance = new calibration_json_store();
    delete instance;
}

fpos_t filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg().seekpos();
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

TEST(calibration_data_store_tests, SaveCalibration)
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
TEST(calibration_data_store_tests, GetCalibration)
{
    calibration_json_store calStore;
    try
    {
        corvis_device_parameters cal;
        calStore.GetCalibration(&cal);
        EXPECT_TRUE(nearlyEqual(testFloat, cal.Fx, FLT_EPSILON));
    }
    catch (runtime_error)
    {
        FAIL();
    }
}

TEST(calibration_data_store_tests, HasCalibration_WrongVersion)
{
    calibration_json_store calStore;
    corvis_device_parameters cal;
    cal.version = CALIBRATION_VERSION - 1;

    try
    {
        calStore.SaveCalibration(cal);
        EXPECT_FALSE(calStore.HasCalibration(DEVICE_TYPE_GIGABYTE_S11));
    }
    catch (runtime_error)
    {
        FAIL();
    }
}

TEST(calibration_data_store_tests, HasCalibration)
{
    calibration_json_store calStore;
    corvis_device_parameters cal;

    try
    {
        // save the defaults as a valid calibration
        get_parameters_for_device(DEVICE_TYPE_GIGABYTE_S11, &cal);
        calStore.SaveCalibration(cal);
        EXPECT_TRUE(calStore.HasCalibration(DEVICE_TYPE_GIGABYTE_S11));
    }
    catch (runtime_error)
    {
        FAIL();
    }
}

TEST(calibration_data_store_tests, HasCalibration_MissingFile)
{
    calibration_json_store calStore;
    try
    {
        calStore.ClearCalibration();
        EXPECT_FALSE(calStore.HasCalibration(DEVICE_TYPE_GIGABYTE_S11));
    }
    catch (runtime_error)
    {
        FAIL();
    }
}
