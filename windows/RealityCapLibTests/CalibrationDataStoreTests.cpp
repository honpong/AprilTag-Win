#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CalibrationDataStore.h"
#include <fstream>
#include <stdio.h>

using namespace RealityCap;

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

TEST(CalibrationDataStoreTests, SaveCalibration)
{
    remove(CALIBRATION_FILE_NAME);

    CalibrationDataStore calStore;
    corvis_device_parameters params;
    params.Fx = .1;
    calStore.SaveCalibration(params);

    // check that file size is greater than zero
    EXPECT_GT(filesize(CALIBRATION_FILE_NAME), 0);
}

TEST(CalibrationDataStoreTests, GetSavedCalibration)
{
    CalibrationDataStore calStore;
    corvis_device_parameters params = calStore.GetSavedCalibration();
    // TODO finish test
}
