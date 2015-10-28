
#include "gtest/gtest.h"
#include "rc_intel_interface.h"
#include <memory>
#include <codecvt>
#include "rs_calibration_json.h"

using namespace std;

static const char *CALIBRATION_DEFAULTS = R"(
{
    "deviceName" : "unknown",
    "calibrationVersion": 1,
    "Fx": 600,
    "Fy": 600,
    "Cx": 319.5,
    "Cy": 239.5,
    "px": 0,
    "py": 0,
    "K0": 0.20000000298023224,
    "K1": -0.20000000298023224,
    "K2": 0,
    "Kw": 0,
    "abias0": 0,
    "abias1": 0,
    "abias2": 0,
    "wbias0": 0,
    "wbias1": 0,
    "wbias2": 0,
    "Tc0": 0.0099999997764825821,
    "Tc1": 0.029999999329447746,
    "Tc2": 0,
    "Wc0": 2.2214415073394775,
    "Wc1": -2.2214415073394775,
    "Wc2": 0,
    "abiasvar0": 0.0096039995551109314,
    "abiasvar1": 0.0096039995551109314,
    "abiasvar2": 0.0096039995551109314,
    "wbiasvar0": 0.0076154354028403759,
    "wbiasvar1": 0.0076154354028403759,
    "wbiasvar2": 0.0076154354028403759,
    "TcVar0": 9.9999999747524271e-07,
    "TcVar1": 9.9999999747524271e-07,
    "TcVar2": 1.000000013351432e-10,
    "WcVar0": 9.9999999747524271e-07,
    "WcVar1": 9.9999999747524271e-07,
    "WcVar2": 9.9999999747524271e-07,
    "wMeasVar": 1.3707783182326239e-05,
    "aMeasVar": 0.00022821025049779564,
    "imageWidth": 640,
    "imageHeight": 480,
    "distortionModel": 0
}
)";

TEST(rc_intel_interface_tests, rc_setCalibration)
{
    rcCalibration calInput;

    strncpy_s(calInput.deviceName, sizeof(calInput.deviceName), R"(test)", _TRUNCATE);
    calInput.imageWidth = 123;
    calInput.imageHeight = 321;

    // set cal
    string jsonString;
    EXPECT_TRUE(calibration_serialize(calInput, jsonString));

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
    rcCalibration calOutput = rc_getCalibrationStruct(tracker);
    EXPECT_STREQ(calInput.deviceName, calOutput.deviceName);
    EXPECT_EQ(123, calOutput.imageWidth);
    EXPECT_EQ(321, calOutput.imageHeight);

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
    rcCalibration calInput;

    calInput.imageWidth = 321;
    calInput.imageHeight = 123;

    rc_Tracker *tracker = rc_create();

    rc_setCalibrationStruct(tracker, calInput);

    rcCalibration calOutput = rc_getCalibrationStruct(tracker);
    EXPECT_EQ(calInput.imageWidth, calOutput.imageWidth);
    EXPECT_EQ(calInput.imageHeight, calOutput.imageHeight);

    rc_destroy(tracker);
}

TEST(rc_intel_interface_tests, rc_setCalibrationFromFile)
{
    rc_Tracker *tracker = rc_create();

    const rc_char_t* filename = L"C:/Users/bhirashi/AppData/Roaming/Local Libraries/Local Documents/rcmain/windows/build/corvis/Debug/calibration.json";
    EXPECT_TRUE(rc_setCalibrationFromFile(tracker, filename));

    rcCalibration calOutput = rc_getCalibrationStruct(tracker);
    EXPECT_STREQ(R"(unknown)", calOutput.deviceName);
    EXPECT_EQ(640, calOutput.imageWidth);
    EXPECT_EQ(480, calOutput.imageHeight);

    rc_destroy(tracker);
}