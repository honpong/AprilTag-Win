#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CaptureManager.h"
#include "PXCSenseManagerFake.h"

using namespace RealityCap;

TEST(CaptureManagerTest, isCapturing)
{
    PXCSenseManagerFake* senseMan = new PXCSenseManagerFake();

    CaptureManager capMan(senseMan);
    EXPECT_FALSE(capMan.isCapturing());

    capMan.StartCapture();
    EXPECT_TRUE(capMan.isCapturing());

    capMan.StopCapture();
    EXPECT_FALSE(capMan.isCapturing());
}