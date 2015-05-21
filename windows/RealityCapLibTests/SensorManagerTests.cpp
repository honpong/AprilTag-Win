#include "stdafx.h"
#include "../../gmock/gtest/gtest.h"
#include "../../gmock/gmock/gmock.h"
#include "SensorManager.h"
#include "PXCSenseManagerFake.h"

using namespace RealityCap;
using ::testing::AtLeast;
using ::testing::A;

bool testComplete = false;

TEST(SensorManagerTests, NewDelete)
{
    SensorManager* sensorMan = new SensorManager(NULL);
    delete sensorMan;
}

TEST(SensorManagerTests, StartStop)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    SensorManager sensorMan(senseMan);
    EXPECT_FALSE(sensorMan.isVideoStreaming());

    EXPECT_TRUE(sensorMan.StartSensors());
    EXPECT_TRUE(sensorMan.isVideoStreaming());

    sensorMan.StopSensors();
    EXPECT_FALSE(sensorMan.isVideoStreaming());
}

TEST(SensorManagerTests, StartFailed)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    SensorManager sensorMan(senseMan);
    senseMan->fakeStatus = PXC_STATUS_ALLOC_FAILED; // causes EnableStreams to return failure
    EXPECT_FALSE(sensorMan.StartSensors());
    EXPECT_FALSE(sensorMan.isVideoStreaming());
}

TEST(SensorManagerTests, NullSenseMan)
{
    SensorManager sensorMan(NULL);
    EXPECT_FALSE(sensorMan.StartSensors());
    EXPECT_FALSE(sensorMan.isVideoStreaming());
}