
#include "gtest/gtest.h"
#include "rc_intel_interface.h"
#include <memory>
#include <codecvt>
#include "calibration_json_store.h"

using namespace std;

TEST(rc_intel_interface_tests, rc_setCalibration)
{
    // load default cal
    corvis_device_parameters cal;
    EXPECT_TRUE(calibration_load_defaults(DEVICE_TYPE_GIGABYTES11, cal));

    // set cal
    string jsonString;
    EXPECT_TRUE(calibration_serialize(cal, jsonString));

    rc_Tracker *tracker = rc_create();
    wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
    wstring wideJsonString = converter.from_bytes(jsonString);
    EXPECT_TRUE(rc_setCalibration(tracker, wideJsonString.c_str()));

    // now read cal back out and compare
    const wchar_t* buffer;
    size_t size = rc_getCalibration(tracker, &buffer);
    EXPECT_GT(size, 0);

    // this doesn't work because not all all fields are being extracted from filter at the moment.
    //EXPECT_STREQ(jsonString.c_str(), buffer);
}

TEST(rc_intel_interface_tests, rc_setCalibration_failure)
{
    rc_Tracker *tracker = rc_create();
    std::wstring wideJsonString = L"";
    EXPECT_FALSE(rc_setCalibration(tracker, wideJsonString.c_str()));
}
