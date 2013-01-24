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
#import "RCAVSessionManagerFake.h"

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
    id<RCAVSessionManager> sessionMan1 = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    id<RCAVSessionManager> sessionMan2 = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    STAssertEqualObjects(sessionMan1, sessionMan2, @"[RCAVSessionManagerFactory getAVSessionManagerInstance] failed to return the same instance");
}

- (void)testSetsAndReturnsSameInstance
{
    id<RCAVSessionManager> sessionMan1 = [[RCAVSessionManagerFake alloc] init];
    
    [RCAVSessionManagerFactory setAVSessionManagerInstance:sessionMan1];
    
    id<RCAVSessionManager> sessionMan2 = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    STAssertEqualObjects(sessionMan1, sessionMan2, @"[RCAVSessionManagerFactory getAVSessionManagerInstance] failed to return the same instance after setAVSessionManagerInstance was called");
}

- (void)testWontStartWithoutBeingSetup
{
    id<RCAVSessionManager> sessionMan = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    STAssertFalse([sessionMan startSession], @"Session started without being setup first");
}

- (void)testStartsAfterBeingSetup
{
    id<RCAVSessionManager> sessionMan = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    [sessionMan setupAVSession];
    
    STAssertTrue([sessionMan startSession], @"Session failed to start after being setup");
}

- (void)testIsRunningAfterStarting
{
    id<RCAVSessionManager> sessionMan = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    [sessionMan setupAVSession];
    [sessionMan startSession];
    
    STAssertTrue([sessionMan isRunning], @"isRunning returned false while session was running");
}

- (void)testIsntRunningAfterEnding
{
    id<RCAVSessionManager> sessionMan = [RCAVSessionManagerFactory getAVSessionManagerInstance];
    
    [sessionMan setupAVSession];
    [sessionMan startSession];
    [sessionMan endSession];
    
    STAssertFalse([sessionMan isRunning], @"Session running after endSession called");
}

@end


