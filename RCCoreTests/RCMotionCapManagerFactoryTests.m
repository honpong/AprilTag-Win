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

- (void)testStartFailsIfPluginsNotStarted
{
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCCorvisManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isPluginsStarted];
    
    id mockMotionMan = [OCMockObject niceMockForClass:[CMMotionManager class]];
    
    id mockOpQueue = [OCMockObject niceMockForClass:[NSOperationQueue class]];
    
    id<RCMotionCapManager> motionMan = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    
    STAssertFalse([motionMan startMotionCap], @"Motion cap started without starting corvis plugins");
}

- (void)testStart
{
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCCorvisManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isPluginsStarted];
    
    id mockOpQueue = [OCMockObject niceMockForClass:[NSOperationQueue class]];
    [[mockOpQueue expect] setMaxConcurrentOperationCount:1];
    
    id mockMotionMan = [OCMockObject mockForClass:[CMMotionManager class]];
    [(CMMotionManager*)[mockMotionMan expect] startAccelerometerUpdatesToQueue:mockOpQueue withHandler:
     ^(CMAccelerometerData *accelerometerData, NSError *error) {
        //dummy
     }];
    
    id<RCMotionCapManager> motionMan = [RCMotionCapManagerFactory getMotionCapManagerInstance];
    
    STAssertFalse([motionMan startMotionCapWithMotionManager:motionMan
                                                   withQueue:mockOpQueue
                                           withCorvisManager:mockCorvisMan],
                  @"Motion cap started without starting corvis plugins"
    );
}

@end
