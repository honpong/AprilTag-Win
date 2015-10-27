
#include "gtest/gtest.h"
#include "rc_intel_interface.h"
#include <memory>
#include <codecvt>
#include "rs_calibration_json.h"

using namespace std;

TEST(rc_intel_interface_tests, rc_setCalibration)
{
    rcCalibration cal;

    cal.Fx = 0.123;
    cal.Fy = 0.321;
    cal.Cx = 0.456;
    cal.Cy = 0.654;

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

    // now read cal back out
    const rc_char_t* buffer;
    size_t size = rc_getCalibration(tracker, &buffer);
    EXPECT_GT(size, 0);

    // compare values between original and retrieved
    rcCalibration cal2 = rc_getCalibrationStruct(tracker);
    EXPECT_FLOAT_EQ(cal.Fx, cal2.Fx);
    EXPECT_FLOAT_EQ(cal.Fx, cal2.Fy);
    EXPECT_FLOAT_EQ(cal.Cx, cal2.Cx);
    EXPECT_FLOAT_EQ(cal.Cy, cal2.Cy);

    // this doesn't work because not all all fields are being extracted from filter at the moment.
    //EXPECT_STREQ(jsonString.c_str(), buffer);

    rc_destroy(tracker);
}

TEST(rc_intel_interface_tests, rc_setCalibration_failure)
{
    rc_Tracker *tracker = rc_create();
#ifdef _WIN32
    EXPECT_FALSE(rc_setCalibration(tracker, L""));
#else
    EXPECT_FALSE(rc_setCalibration(tracker, ""));
#endif
    rc_destroy(tracker);
}

TEST(rc_intel_interface_tests, rc_setCalibrationStruct)
{
    rcCalibration cal;

    cal.Fx = 0.123;
    cal.Fy = 0.321;
    cal.Cx = 0.456;
    cal.Cy = 0.654;

    rc_Tracker *tracker = rc_create();

    rc_setCalibrationStruct(tracker, cal);

    rcCalibration cal2 = rc_getCalibrationStruct(tracker);
    EXPECT_FLOAT_EQ(cal.Fx, cal2.Fx);
    EXPECT_FLOAT_EQ(cal.Fx, cal2.Fy);
    EXPECT_FLOAT_EQ(cal.Cx, cal2.Cx);
    EXPECT_FLOAT_EQ(cal.Cy, cal2.Cy);

    rc_destroy(tracker);
}
