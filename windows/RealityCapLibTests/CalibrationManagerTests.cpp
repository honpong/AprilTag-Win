#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CalibrationManager.h"
#include "PXCSenseManagerFake.h"

using namespace RealityCap;

TEST(CalibrationManagerTest, isCalibrating)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    CalibrationManager calMan(senseMan);
    EXPECT_FALSE(calMan.isCalibrating());

    calMan.StartCalibration();
    EXPECT_TRUE(calMan.isCalibrating());

    calMan.StopCalibration();
    EXPECT_FALSE(calMan.isCalibrating());
}