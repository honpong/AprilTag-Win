#include "gtest/gtest.h"
#include "device_parameters.h"
#include "calibration_json.h"
#include "calibration_defaults.h"
#include <memory>

using namespace std;

TEST(device_parameters_tests, get_parameters_for_device)
{
    device_parameters cal;
    get_parameters_for_device(DEVICE_TYPE_GIGABYTES11, &cal);
    EXPECT_EQ(cal.version, CALIBRATION_VERSION);
}

TEST(device_parameters_tests, device_set_resolution)
{
    device_parameters cal;
    device_set_resolution(&cal, 96, 69);

    EXPECT_EQ(96, cal.color.intrinsics.width_px);
    EXPECT_EQ(69, cal.color.intrinsics.height_px);
}
