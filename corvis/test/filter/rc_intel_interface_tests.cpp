
#include "gtest/gtest.h"
#include "rc_intel_interface.h"
#include <memory>

using namespace std;

TEST(rc_intel_interface_tests, rc_getCalibration)
{
    rc_Tracker *tracker = rc_create();
    wchar_t* buffer;
    size_t size = rc_getCalibration(tracker, &buffer);
    EXPECT_TRUE(size);

    EXPECT_TRUE(rc_setCalibration(tracker, buffer));
}
