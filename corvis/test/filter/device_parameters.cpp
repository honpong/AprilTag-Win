#include "gtest/gtest.h"
#include "device_parameters.h"
#include "calibration_data_store.h"
#include <memory>

using namespace std;
using namespace RealityCap;

TEST(device_parameters_tests, get_device_type_string)
{
    string s = get_device_type_string(DEVICE_TYPE_GIGABYTE_S11);
    EXPECT_STRCASEEQ(s.c_str(), "gigabyte_s11");
}

TEST(device_parameters_tests, get_device_type_string_map)
{
    unordered_map<corvis_device_type, string> map;
    get_device_type_string_map(map);
    EXPECT_GT(map.size(), 0);
}

TEST(device_parameters_tests, get_device_by_name)
{
    corvis_device_type type = get_device_by_name("gigabyte_s11");
    EXPECT_EQ(DEVICE_TYPE_GIGABYTE_S11, type);
}

TEST(device_parameters_tests, get_parameters_for_device)
{
    corvis_device_parameters cal;
    get_parameters_for_device(DEVICE_TYPE_GIGABYTE_S11, &cal);
    EXPECT_EQ(cal.version, CALIBRATION_VERSION);
}

TEST(device_parameters_tests, get_parameters_for_device_name)
{
    corvis_device_parameters cal;
    get_parameters_for_device_name("gigabyte_s11", &cal);
    EXPECT_EQ(cal.version, CALIBRATION_VERSION);
    // TODO: verify correct file was loaded?
}

TEST(device_parameters_tests, is_calibration_valid)
{
    unique_ptr<calibration_data_store> store = calibration_data_store::GetStore();

    corvis_device_parameters cal;
    store->LoadCalibrationDefaults(DEVICE_TYPE_GIGABYTE_S11, cal);

    corvis_device_parameters defaults;
    store->LoadCalibrationDefaults(DEVICE_TYPE_GIGABYTE_S11, defaults);

    EXPECT_TRUE(is_calibration_valid(cal, defaults));
}

TEST(device_parameters_tests, is_calibration_valid_not)
{
    unique_ptr<calibration_data_store> store = calibration_data_store::GetStore();

    corvis_device_parameters cal;
    store->LoadCalibrationDefaults(DEVICE_TYPE_IPAD2, cal);
    cal.a_bias[1] = 999; // makes is_calibration_valid() return false

    corvis_device_parameters defaults;
    store->LoadCalibrationDefaults(DEVICE_TYPE_IPAD2, defaults);

    EXPECT_FALSE(is_calibration_valid(cal, defaults));
}

TEST(device_parameters_tests, is_calibration_valid_wrong_version)
{
    unique_ptr<calibration_data_store> store = calibration_data_store::GetStore();

    corvis_device_parameters cal;
    store->LoadCalibrationDefaults(DEVICE_TYPE_IPAD2, cal);
    cal.version = cal.version - 1; // makes is_calibration_valid() return false

    corvis_device_parameters defaults;
    store->LoadCalibrationDefaults(DEVICE_TYPE_IPAD2, defaults);

    EXPECT_FALSE(is_calibration_valid(cal, defaults));
}

TEST(device_parameters_tests, device_set_resolution)
{
    corvis_device_parameters cal;
    device_set_resolution(&cal, 96, 69);

    EXPECT_EQ(96, cal.image_width);
    EXPECT_EQ(69, cal.image_height);
}

TEST(device_parameters_tests, device_set_framerate)
{
    corvis_device_parameters cal;
    device_set_framerate(&cal, 99.f);

    // don't test for specific values, which may be brittle. just make sure it's something reasonable.
    EXPECT_GE(cal.shutter_delay.count(), 0); 
    EXPECT_GT(cal.shutter_period.count(), 0);
}