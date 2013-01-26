//
//  RCCoreTests.m
//  RCCoreTests
//
//  Created by Ben Hirashima on 10/10/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCAVSessionManagerFactoryTests.h"
#import "RCDistanceFormatter.h"
#import "RCAVSessionManagerFactory.h"
#import <OCMock.h>

@implementation RCAVSessionManagerFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    [RCAVSessionManagerFactory setAVSessionManagerInstance:nil];
    
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    [RCAVSessionManagerFactory setupAVSession];
    
    id<RCAVSessionManager> sessionMan1 = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    id<RCAVSessionManager> sessionMan2 = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    STAssertEqualObjects(sessionMan1, sessionMan2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    id<RCAVSessionManager> sessionMan1 = [OCMockObject mockForProtocol:@protocol(RCAVSessionManager)];
    
    [RCAVSessionManagerFactory setAVSessionManagerInstance:sessionMan1];
    
    id<RCAVSessionManager> sessionMan2 = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    STAssertEqualObjects(sessionMan1, sessionMan2, @"Get instance failed to return the same instance after set instance was called");
}

- (void)testGetInstanceWithoutSetupFails
{
    id<RCAVSessionManager> sessionMan = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    STAssertNil(sessionMan, @"Instance requested without being setup first");
}

- (void)testStartSession
{
    [RCAVSessionManagerFactory setupAVSession];
    id<RCAVSessionManager> sessionMan = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    STAssertTrue([sessionMan startSession], @"Session failed to start after being setup");
}

- (void)testIsRunningAfterStarting
{
    [RCAVSessionManagerFactory setupAVSession];
    id<RCAVSessionManager> sessionMan = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    [sessionMan startSession];
    
    STAssertTrue([sessionMan isRunning], @"isRunning returned false while session was running");
}

- (void)testIsntRunningAfterEnding
{
    [RCAVSessionManagerFactory setupAVSession];
    id<RCAVSessionManager> sessionMan = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    [sessionMan startSession];
    [sessionMan endSession];
    
    STAssertFalse([sessionMan isRunning], @"Session running after endSession called");
}

@end


