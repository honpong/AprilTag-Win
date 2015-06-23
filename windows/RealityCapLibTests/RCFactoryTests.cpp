#include "stdafx.h"
#include "../../gtest/gtest.h"
#include "RCFactory.h"
#include "PXCSenseManagerFake.h"

using namespace RealityCap;

TEST(RCFactoryTests, NewDelete)
{
    RCFactory* factory = new RCFactory();
    delete factory;
}
