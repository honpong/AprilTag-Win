#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "CaptureManager.h"
#include "RCFactory.h"
#include "PXCSenseManagerFake.h"

using namespace RealityCap;

TEST(RCFactoryTests, NewDelete)
{
    RCFactory* factory = new RCFactory();
    delete factory;
}

TEST(RCFactoryTests, SameSenseMan)
{
    RCFactory factory;
    unique_ptr<CaptureManager> capMan = factory.CreateCaptureManager();
    ASSERT_EQ(calMan->GetSenseManager(), capMan->GetSenseManager());
}
