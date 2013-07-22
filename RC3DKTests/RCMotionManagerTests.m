//
//  RCMotionCapManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCMotionManagerTests.h"
#import "RCMotionManager.h"
#import "OCMock.h"
#import "RCSensorFusion+RCSensorFusionMock.h"

@implementation RCMotionManagerTests

- (void)setUp
{
    [super setUp];
    // Set-up code here.
}

- (void)tearDown
{
    [RCSensorFusion tearDownMock];
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    RCMotionManager* motionMan1 = [RCMotionManager sharedInstance];
    RCMotionManager* motionMan2 = [RCMotionManager sharedInstance];
    
    STAssertEqualObjects(motionMan1, motionMan2, @"Get instance failed to return the same instance");
}

- (void)testStart
{
    [RCSensorFusion setupNiceMock];
    id mockSensorFusion = [RCSensorFusion sharedInstance];
    [[[mockSensorFusion stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isSensorFusionRunning];
    id mockOpQueue = [OCMockObject mockForClass:[NSOperationQueue class]];
    [[mockOpQueue expect] setMaxConcurrentOperationCount:1];
    
    id mockCMMotionMan = [OCMockObject niceMockForClass:[CMMotionManager class]];
    [(CMMotionManager*)[mockCMMotionMan expect] startAccelerometerUpdatesToQueue:mockOpQueue withHandler:[OCMArg any]];
    [(CMMotionManager*)[mockCMMotionMan expect] startGyroUpdatesToQueue:mockOpQueue withHandler:[OCMArg any]];
    
    RCMotionManager* motionMan = [RCMotionManager sharedInstance];
    motionMan.cmMotionManager = mockCMMotionMan;
    
    STAssertTrue([motionMan startMotionCapWithQueue:mockOpQueue], @"Motion cap failed to start");
    STAssertTrue([motionMan isCapturing], @"isCapturing returned false after started");
    
    [mockCMMotionMan verify];
    [mockOpQueue verify];
}

- (void)testStop
{
    [RCSensorFusion setupNiceMock];
    id mockSensorFusion = [RCSensorFusion sharedInstance];
    [[[mockSensorFusion stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isSensorFusionRunning];
    id mockOpQueue = [OCMockObject niceMockForClass:[NSOperationQueue class]];
    id mockCMMotionMan = [OCMockObject niceMockForClass:[CMMotionManager class]];
    
    RCMotionManager* motionMan = [RCMotionManager sharedInstance];
    motionMan.cmMotionManager = mockCMMotionMan;
    
    STAssertTrue([motionMan startMotionCapWithQueue:mockOpQueue], @"Motion cap failed to start");
    
    [[[mockCMMotionMan stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isAccelerometerActive];
    [[[mockCMMotionMan stub] andReturnValue:OCMOCK_VALUE((BOOL) {YES})] isGyroActive];
    
    [[mockOpQueue expect] cancelAllOperations];
    [[mockCMMotionMan expect] stopAccelerometerUpdates];
    [[mockCMMotionMan expect] stopGyroUpdates];

    [motionMan stopMotionCapture];
    
    STAssertFalse([motionMan isCapturing], @"isCapturing returned true after stopped");
    
    [mockOpQueue verify];
    [mockCMMotionMan verify];
}

@end
