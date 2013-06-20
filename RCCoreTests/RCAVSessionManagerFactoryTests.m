//
//  RCCoreTests.m
//  RCCoreTests
//
//  Created by Ben Hirashima on 10/10/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCAVSessionManagerFactoryTests.h"
#import "RCAVSessionManager.h"
#import <OCMock.h>

@implementation RCAVSessionManagerFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    [RCAVSessionManager setInstance:nil];
    
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    [RCAVSessionManager setupAVSession];
    
    id<RCAVSessionManager> sessionMan1 = [RCAVSessionManager getInstance];
    id<RCAVSessionManager> sessionMan2 = [RCAVSessionManager getInstance];
    
    STAssertEqualObjects(sessionMan1, sessionMan2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    id<RCAVSessionManager> sessionMan1 = [OCMockObject mockForProtocol:@protocol(RCAVSessionManager)];

    [RCAVSessionManager setInstance:sessionMan1];
    
    id<RCAVSessionManager> sessionMan2 = [RCAVSessionManager getInstance];
    
    STAssertEqualObjects(sessionMan1, sessionMan2, @"Get instance failed to return the same instance after set instance was called");
}

- (void)testGetInstanceWithoutSetupFails
{
    id<RCAVSessionManager> sessionMan = [RCAVSessionManager getInstance];
    
    STAssertNil(sessionMan, @"Instance requested without being setup first");
}

- (void)testStartSession
{
    [RCAVSessionManager setupAVSession];
    id<RCAVSessionManager> sessionMan = [RCAVSessionManager getInstance];
    
    STAssertTrue([sessionMan startSession], @"Session failed to start after being setup");
}

- (void)testIsRunningAfterStarting
{
    [RCAVSessionManager setupAVSession];
    id<RCAVSessionManager> sessionMan = [RCAVSessionManager getInstance];
    
    [sessionMan startSession];
    
    STAssertTrue([sessionMan isRunning], @"isRunning returned false while session was running");
}

- (void)testIsntRunningAfterEnding
{
    [RCAVSessionManager setupAVSession];
    id<RCAVSessionManager> sessionMan = [RCAVSessionManager getInstance];
    
    [sessionMan startSession];
    [sessionMan endSession];
    
    STAssertFalse([sessionMan isRunning], @"Session running after endSession called");
}

@end


