#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CalibrationManager.h"
#include "pxcsensemanager.h"

using namespace RealityCap;

TEST(CalibrationManagerTest, isCalibrating)
{
    PXCSenseManager* senseMan = PXCSenseManager::CreateInstance();

    CalibrationManager calMan(senseMan);
    EXPECT_FALSE(calMan.isCalibrating());

    calMan.StartCalibration();
    EXPECT_TRUE(calMan.isCalibrating());

    calMan.StopCalibration();
    EXPECT_FALSE(calMan.isCalibrating());
}