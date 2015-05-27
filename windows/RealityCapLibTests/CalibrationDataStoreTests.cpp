#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CalibrationDataStore.h"
#include <fstream>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace RealityCap;
using namespace std;

const float testFloat = .1;

TEST(CalibrationDataStoreTests, NewDelete)
{
    CalibrationDataStore* instance = new CalibrationDataStore();
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

    if (a == b) { // shortcut, handles infinities
        return true;
    }
    else if (a == 0 || b == 0 || diff < FLT_MIN) {
        // a or b is zero or both are extremely close to it
        // relative error is less meaningful here
        return diff < (epsilon * FLT_MIN);
    }
    else { // use relative error
        return diff / fmin((absA + absB), FLT_MAX) < epsilon;
    }
}

void createDefaultCalibrationFile()
{
    remove(CALIBRATION_DEFAULTS_FILE_NAME);
    wstring json = L"{\"Cx\":-107374176,\"Cy\":-107374176,\"Fx\":0.10000000149011612,\"Fy\":-107374176,\"K0\":-107374176,\"K1\":-107374176,\"K2\":-107374176,\"Tc0\":-107374176,\"Tc1\":-107374176,\"Tc2\":-107374176,\"TcVar0\":-107374176,\"TcVar1\":-107374176,\"TcVar2\":-107374176,\"Wc0\":-107374176,\"Wc1\":-107374176,\"Wc2\":-107374176,\"WcVar0\":-107374176,\"WcVar1\":-107374176,\"WcVar2\":-107374176,\"aMeasVar\":-107374176,\"abias0\":-107374176,\"abias1\":-107374176,\"abias2\":-107374176,\"abiasvar0\":-107374176,\"abiasvar1\":-107374176,\"abiasvar2\":-107374176,\"calibrationVersion\":7,\"imageHeight\":-858993460,\"imageWidth\":-858993460,\"px\":-107374176,\"py\":-107374176,\"shutterDelay\":-3.6893488147419104e+17,\"shutterPeriod\":-3.6893488147419104e+17,\"wMeasVar\":-107374176,\"wbias0\":-107374176,\"wbias1\":-107374176,\"wbias2\":-107374176,\"wbiasvar0\":-107374176,\"wbiasvar1\":-107374176,\"wbiasvar2\":-107374176}";
    
    wofstream jsonFile;
    jsonFile.open(CALIBRATION_DEFAULTS_FILE_NAME);
    jsonFile << json;
    jsonFile.close();
}

TEST(CalibrationDataStoreTests, SaveCalibration)
{
    remove(CALIBRATION_FILE_NAME);

    CalibrationDataStore calStore;
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
TEST(CalibrationDataStoreTests, GetCalibration)
{
    CalibrationDataStore calStore;
    try
    {
        corvis_device_parameters cal = calStore.GetCalibration();
        EXPECT_TRUE(nearlyEqual(testFloat, cal.Fx, FLT_EPSILON));
    }
    catch (runtime_error)
    {
        FAIL();
    }
}

TEST(CalibrationDataStoreTests, HasCalibration_MissingFile)
{
    CalibrationDataStore calStore;
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

TEST(CalibrationDataStoreTests, HasCalibration_WrongVersion)
{
    CalibrationDataStore calStore;
    corvis_device_parameters cal;
    cal.version = CALIBRATION_VERSION - 1;

    try
    {
        calStore.SaveCalibration(cal);
        EXPECT_FALSE(calStore.HasCalibration());
    }
    catch (runtime_error)
    {
        FAIL();
    }
}

TEST(CalibrationDataStoreTests, HasCalibration)
{
    CalibrationDataStore calStore;
    corvis_device_parameters cal;

    try
    {
        // save the defaults as a valid calibration
        get_parameters_for_device(DEVICE_TYPE_GIGABYTE_S11, &cal);
        calStore.SaveCalibration(cal);
        EXPECT_TRUE(calStore.HasCalibration());
    }
    catch (runtime_error)
    {
        FAIL();
    }
}
