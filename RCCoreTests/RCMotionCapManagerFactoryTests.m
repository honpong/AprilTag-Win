//
//  RCMotionCapManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMotionCapManagerFactoryTests.h"
#import "RCMotionCapManagerFactory.h"
#import <OCMock.h>

@implementation RCMotionCapManagerFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    [RCMotionCapManagerFactory setMotionCapManagerInstance:nil];
    
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    id<RCMotionCapManager> motionMan1 = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    id<RCMotionCapManager> motionMan2 = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    
    STAssertEqualObjects(motionMan1, motionMan2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    id motionMan1 = [OCMockObject mockForProtocol:@protocol(RCMotionCapManager)];
    
    [RCMotionCapManagerFactory setMotionCapManagerInstance:motionMan1];
    
    id motionMan2 = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    
    STAssertEqualObjects(motionMan1, motionMan2, @"Get instance failed to return the same instance after set instance was called");
}

//- (void)testStartFailsIfPluginsNotStarted
//{
//    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCCorvisManager)];
//    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isPluginsStarted];
//    
//    id mockMotionMan = [OCMockObject niceMockForClass:[]
//}

@end
