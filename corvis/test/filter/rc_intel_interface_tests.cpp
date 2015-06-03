
#include "gtest/gtest.h"
#include "rc_intel_interface.h"
#include <memory>
#include "calibration_json_store.h"

using namespace std;
using namespace RealityCap;

TEST(rc_intel_interface_tests, rc_getCalibration)
{
    rc_Tracker *tracker = rc_create();
    const wchar_t* buffer;
    size_t size = rc_getCalibration(tracker, &buffer);
    EXPECT_TRUE(size);
}

TEST(rc_intel_interface_tests, rc_setCalibration)
{
    unique_ptr<calibration_data_store> store = calibration_data_store::GetStore();
    corvis_device_parameters cal;
    EXPECT_TRUE(store->LoadCalibrationDefaults(DEVICE_TYPE_GIGABYTE_S11, cal));

    wstring jsonString;
    EXPECT_TRUE(calibration_json_store::SerializeCalibration(cal, jsonString));

    rc_Tracker *tracker = rc_create();
    EXPECT_TRUE(rc_setCalibration(tracker, jsonString.c_str()));
}
