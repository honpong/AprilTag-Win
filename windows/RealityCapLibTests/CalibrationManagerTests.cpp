#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CalibrationManager.h"
#include "PXCSenseManagerFake.h"

using namespace RealityCap;

TEST(CalibrationManagerTests, NewDelete)
{
    CalibrationManager* calMan = new CalibrationManager(NULL);
    delete calMan;
}

TEST(CalibrationManagerTests, StartStop)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    CalibrationManager calMan(senseMan);
    EXPECT_FALSE(calMan.isCalibrating());

    EXPECT_TRUE(calMan.StartCalibration());
    EXPECT_TRUE(calMan.isCalibrating());

    calMan.StopCalibration();
    EXPECT_FALSE(calMan.isCalibrating());
}

TEST(CalibrationManagerTests, StartFailed)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();
    CalibrationManager calMan(senseMan);
    senseMan->fakeStatus = PXC_STATUS_ALLOC_FAILED; // causes EnableStreams to return failure
    EXPECT_FALSE(calMan.StartCalibration());
    EXPECT_FALSE(calMan.isCalibrating());
}

TEST(CalibrationManagerTests, NullSenseMan)
{
    CalibrationManager calMan(NULL);
    EXPECT_FALSE(calMan.StartCalibration());
    EXPECT_FALSE(calMan.isCalibrating());
}