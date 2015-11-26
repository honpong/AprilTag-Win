
#include "gtest/gtest.h"
#include "rc_intel_interface.h"
#include <memory>
#include <codecvt>
#include "calibration_json.h"
#include "json_keys.h"

using namespace std;

TEST(rc_intel_interface_tests, rc_setCalibration)
{
    rcCalibration calInput, defaults;

    snprintf(calInput.deviceName, sizeof(calInput.deviceName), "%s", R"(test)");
    calInput.image_width = 123;
    calInput.image_height = 321;
    calInput.px = 0.567;
    calInput.py = 0.765;
    calInput.distortionModel = 1;
    calInput.accelerometerTransform[0] = 0.123;
    calInput.accelerometerTransform[8] = 0.321;
    calInput.gyroscopeTransform[0] = 0.456;
    calInput.gyroscopeTransform[8] = 0.654;
    calInput.shutterDelay = 0.666;
    calInput.shutterPeriod = 0.777;
    //calInput.a_bias_var[2] = 1.111111111e-06;
    //calInput.w_bias_var[2] = 2.222222222e-07;

    // set cal
    string jsonString;
    EXPECT_TRUE(calibration_serialize(calInput, jsonString));

    rc_Tracker *tracker = rc_create();
#ifdef _WIN32
    wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
    EXPECT_TRUE(rc_setCalibration(tracker, converter.from_bytes(jsonString).c_str(), &defaults));
#else
    EXPECT_TRUE(rc_setCalibration(tracker, jsonString.c_str(), &defaults));
#endif

    // now read cal back out
    const rc_char_t* buffer;
    size_t size = rc_getCalibration(tracker, &buffer);
    EXPECT_GT(size, 0);

    // compare values between original and retrieved
    rcCalibration calOutput;
    rc_getCalibrationStruct(tracker, &calOutput);
    EXPECT_STREQ(calInput.deviceName, calOutput.deviceName);
    EXPECT_EQ(calInput.image_width, calOutput.image_width);
    EXPECT_EQ(calInput.image_height, calOutput.image_height);
    EXPECT_FLOAT_EQ(calInput.px, calOutput.px);
    EXPECT_FLOAT_EQ(calInput.py, calOutput.py);
    EXPECT_EQ(calInput.distortionModel, calOutput.distortionModel);
    EXPECT_FLOAT_EQ(calInput.shutterDelay, calOutput.shutterDelay);
    EXPECT_FLOAT_EQ(calInput.shutterPeriod, calOutput.shutterPeriod);
    //EXPECT_FLOAT_EQ(calInput.a_bias_var[2], calOutput.a_bias_var[2]); these fail because there are minimums lower than the input values. not a problem.
    //EXPECT_FLOAT_EQ(calInput.w_bias_var[2], calOutput.w_bias_var[2]);
    EXPECT_FLOAT_EQ(calInput.accelerometerTransform[0], calOutput.accelerometerTransform[0]);
    EXPECT_FLOAT_EQ(calInput.accelerometerTransform[8], calOutput.accelerometerTransform[8]);
    EXPECT_FLOAT_EQ(calInput.gyroscopeTransform[0], calOutput.gyroscopeTransform[0]);
    EXPECT_FLOAT_EQ(calInput.gyroscopeTransform[8], calOutput.gyroscopeTransform[8]);

    // this doesn't work because not all all fields are being extracted from filter at the moment.
    //EXPECT_STREQ(jsonString.c_str(), buffer);

    rc_destroy(tracker);
}

TEST(rc_intel_interface_tests, rc_setCalibration_failure)
{
    rcCalibration defaults;
    rc_Tracker *tracker = rc_create();
#ifdef _WIN32
    EXPECT_FALSE(rc_setCalibration(tracker, L"", &defaults));
#else
    EXPECT_FALSE(rc_setCalibration(tracker, "", &defaults));
#endif
    rc_destroy(tracker);
}

TEST(rc_intel_interface_tests, rc_setCalibrationStruct)
{
    rcCalibration calInput;

    calInput.image_width = 321;
    calInput.image_height = 123;

    rc_Tracker *tracker = rc_create();

    rc_setCalibrationStruct(tracker, &calInput);

    rcCalibration calOutput;
    rc_getCalibrationStruct(tracker, &calOutput);
    EXPECT_EQ(calInput.image_width, calOutput.image_width);
    EXPECT_EQ(calInput.image_height, calOutput.image_height);

    rc_destroy(tracker);
}

//TEST(rc_intel_interface_tests, rc_setCalibrationFromFile)
//{
//    rc_Tracker *tracker = rc_create();
//
//    const rc_char_t* filename = L"C:/Users/bhirashi/Documents/gigabyte_s11.json";
//    EXPECT_TRUE(rc_setCalibrationFromFile(tracker, filename));
//
//    rcCalibration calOutput;
//    rc_getCalibrationStruct(tracker, &calOutput);
//    EXPECT_STREQ(R"(gigabyte_s11)", calOutput.deviceName);
//    EXPECT_EQ(640, calOutput.image_width);
//    EXPECT_EQ(480, calOutput.image_height);
//    EXPECT_FLOAT_EQ(0.1, calOutput.shutterDelay);
//    EXPECT_FLOAT_EQ(0.2, calOutput.shutterPeriod);
//    EXPECT_FLOAT_EQ(0.3, calOutput.timeStampOffset);
//    EXPECT_FLOAT_EQ(-9.80665, calOutput.accelerometerTransform[1]);
//    EXPECT_FLOAT_EQ(0.017453292519943295, calOutput.gyroscopeTransform[0]);
//
//    rc_destroy(tracker);
//}

TEST(rc_intel_interface_tests, rc_fisheyeKw)
{
    rcCalibration calInput;

    calInput.Kw = .123;
    calInput.distortionModel = 1;

    rc_Tracker *tracker = rc_create();

    rc_setCalibrationStruct(tracker, &calInput);

    rcCalibration calOutput;
    rc_getCalibrationStruct(tracker, &calOutput);
    EXPECT_FLOAT_EQ(calInput.Kw, calOutput.K0);
    EXPECT_FLOAT_EQ(calInput.Kw, calOutput.Kw);
    EXPECT_EQ(calInput.distortionModel, calOutput.distortionModel);

    rc_destroy(tracker);
}

TEST(rc_intel_interface_tests, calibration_defaults)
{
    rc_Tracker *tracker = rc_create();
    rcCalibration defaults;

    snprintf(defaults.deviceName, sizeof(defaults.deviceName), "%s", R"(test)");
    defaults.image_width = 456;
    defaults.image_height = 321;
    defaults.shutterDelay = 0.789;
    defaults.accelerometerTransform[7] = 0.666; // this should not be set, since JSON contains a value for it
    defaults.accelerometerTransform[8] = 0.777; // this should be set, since last element is missing in JSON

    std::stringstream jsonString;
    jsonString << "{ \"" << KEY_IMAGE_WIDTH << "\" : 123, \"" << KEY_ACCEL_TRANSFORM << "\" : [0,0,0,0,0,0,0,0.6] }"; // note that last element of array is missing

#ifdef _WIN32
    wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
    EXPECT_TRUE(rc_setCalibration(tracker, converter.from_bytes(jsonString.str()).c_str(), &defaults));
#else
    EXPECT_TRUE(rc_setCalibration(tracker, jsonString.str().c_str(), &defaults));
#endif
    
    // compare values between original and retrieved
    rcCalibration calOutput;
    rc_getCalibrationStruct(tracker, &calOutput);

    // check that default value was NOT used
    EXPECT_EQ(123, calOutput.image_width); 
    EXPECT_FLOAT_EQ(calOutput.accelerometerTransform[7], 0.6);

    // check that default values were set
    EXPECT_STREQ("test", calOutput.deviceName);
    EXPECT_EQ(defaults.image_height, calOutput.image_height);
    EXPECT_EQ(defaults.shutterDelay, calOutput.shutterDelay);
    EXPECT_FLOAT_EQ(calOutput.accelerometerTransform[8], defaults.accelerometerTransform[8]);
    
    rc_destroy(tracker);
}
