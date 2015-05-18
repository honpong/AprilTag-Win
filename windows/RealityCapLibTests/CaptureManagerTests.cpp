#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CaptureManager.h"
#include "pxcsensemanager.h"

using namespace RealityCap;

TEST(CaptureManagerTest, isCapturing)
{
    PXCSenseManager* senseMan = PXCSenseManager::CreateInstance();

    CaptureManager capMan(senseMan);
    EXPECT_FALSE(capMan.isCapturing());

    capMan.StartCapture();
    EXPECT_TRUE(capMan.isCapturing());

    capMan.StopCapture();
    EXPECT_FALSE(capMan.isCapturing());
}