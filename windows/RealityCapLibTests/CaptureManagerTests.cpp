#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CaptureManager.h"
#include "PXCSenseManagerFake.h"

using namespace RealityCap;

TEST(CaptureManagerTest, NewDelete)
{
    CaptureManager* capMan = new CaptureManager(NULL);
    delete capMan;
}

TEST(CaptureManagerTest, StartStop)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    CaptureManager capMan(senseMan);
    EXPECT_FALSE(capMan.isCapturing());

    EXPECT_TRUE(capMan.StartCapture());
    EXPECT_TRUE(capMan.isCapturing());

    capMan.StopCapture();
    EXPECT_FALSE(capMan.isCapturing());
}

TEST(CaptureManagerTest, StartFailed)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    CaptureManager capMan(senseMan);
    senseMan->FakeStatus = PXC_STATUS_ALLOC_FAILED; // causes EnableStreams to return failure
    EXPECT_FALSE(capMan.StartCapture());
    EXPECT_FALSE(capMan.isCapturing());
}

TEST(CaptureManagerTest, NullSenseMan)
{
    CaptureManager capMan(NULL);
    EXPECT_FALSE(capMan.StartCapture());
    EXPECT_FALSE(capMan.isCapturing());
}