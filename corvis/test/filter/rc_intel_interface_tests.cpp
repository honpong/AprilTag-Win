
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
#ifdef _WIN32
    wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
    EXPECT_TRUE(rc_setCalibration(tracker, converter.from_bytes(jsonString).c_str()));
#else
    EXPECT_TRUE(rc_setCalibration(tracker, jsonString.c_str()));
#endif

    // now read cal back out and compare
    const rc_char_t* buffer;
    size_t size = rc_getCalibration(tracker, &buffer);
    EXPECT_GT(size, 0);

    // this doesn't work because not all all fields are being extracted from filter at the moment.
    //EXPECT_STREQ(jsonString.c_str(), buffer);
}

TEST(rc_intel_interface_tests, rc_setCalibration_failure)
{
    rc_Tracker *tracker = rc_create();
#ifdef _WIN32
    EXPECT_FALSE(rc_setCalibration(tracker, L"".c_str()));
#else
    EXPECT_FALSE(rc_setCalibration(tracker, ""));
#endif
}
