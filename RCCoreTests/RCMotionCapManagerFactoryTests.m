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
    [RCMotionCapManagerFactory setInstance:nil];
    
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    id<RCMotionCapManager> motionMan1 = [RCMotionCapManagerFactory getInstance];
    id<RCMotionCapManager> motionMan2 = [RCMotionCapManagerFactory getInstance];
    
    STAssertEqualObjects(motionMan1, motionMan2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    id motionMan1 = [OCMockObject mockForProtocol:@protocol(RCMotionCapManager)];
    
    [RCMotionCapManagerFactory setInstance:motionMan1];
    
    id motionMan2 = [RCMotionCapManagerFactory getInstance];
    
    STAssertEqualObjects(motionMan1, motionMan2, @"Get instance failed to return the same instance after set instance was called");
}

- (void)testStartFailsIfPluginsNotStarted
{
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCPimManager)];
    [RCSensorFusion setInstance:mockCorvisMan];
    
    id mockMotionMan = [OCMockObject niceMockForClass:[CMMotionManager class]];
    
    id mockOpQueue = [OCMockObject niceMockForClass:[NSOperationQueue class]];
    
    id<RCMotionCapManager> motionMan = [RCMotionCapManagerFactory getInstance];
    
    STAssertFalse([motionMan startMotionCapWithQueue:mockOpQueue],
          @"Motion cap started without starting corvis plugins"
    );
}

- (void)testStart
{
    id mockCorvisMan = [OCMockObject mockForProtocol:@protocol(RCPimManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isPluginsStarted];
    [RCSensorFusion setInstance:mockCorvisMan];
    
    id mockOpQueue = [OCMockObject mockForClass:[NSOperationQueue class]];
    [[mockOpQueue expect] setMaxConcurrentOperationCount:1];
    
    id mockCMMotionMan = [OCMockObject niceMockForClass:[CMMotionManager class]];
//    [(CMMotionManager*)[mockCMMotionMan expect] setAccelerometerUpdateInterval:.01]; //makes test brittle?
//    [(CMMotionManager*)[mockCMMotionMan expect] setGyroUpdateInterval:.01];
    [(CMMotionManager*)[mockCMMotionMan expect] startAccelerometerUpdatesToQueue:mockOpQueue withHandler:[OCMArg any]];
    [(CMMotionManager*)[mockCMMotionMan expect] startGyroUpdatesToQueue:mockOpQueue withHandler:[OCMArg any]];
    
    id<RCMotionCapManager> motionMan = [RCMotionCapManagerFactory getInstance];
    
    STAssertTrue([motionMan startMotionCapWithQueue:mockOpQueue], @"Motion cap failed to start");
    STAssertTrue([motionMan isCapturing], @"isCapturing returned false after started");
    
    [mockCMMotionMan verify];
    [mockOpQueue verify];
}

- (void)testStop
{
    id mockCorvisMan = [OCMockObject niceMockForProtocol:@protocol(RCPimManager)];
    [[[mockCorvisMan stub] andReturnValue:OCMOCK_VALUE((BOOL){YES})] isPluginsStarted];
    [RCSensorFusion setInstance:mockCorvisMan];
    
    id mockOpQueue = [OCMockObject niceMockForClass:[NSOperationQueue class]];
       
    id mockCMMotionMan = [OCMockObject niceMockForClass:[CMMotionManager class]];
    
    id<RCMotionCapManager> motionMan = [RCMotionCapManagerFactory getInstance];
    
    STAssertTrue([motionMan startMotionCapWithQueue:mockOpQueue], @"Motion cap failed to start");
    
    [[[mockCMMotionMan stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isAccelerometerActive];
    [[[mockCMMotionMan stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isGyroActive];
    
    [[mockOpQueue expect] cancelAllOperations];
    [[mockCMMotionMan expect] stopAccelerometerUpdates];
    [[mockCMMotionMan expect] stopGyroUpdates];
    
    [motionMan stopMotionCap];
    
    STAssertFalse([motionMan isCapturing], @"isCapturing returned true after stopped");
    
    [mockOpQueue verify];
    [mockCMMotionMan verify];
}

@end
