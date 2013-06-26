//
//  RCCoreTests.m
//  RCCoreTests
//
//  Created by Ben Hirashima on 10/10/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCAVSessionManagerTests.h"
#import "RCAVSessionManager.h"

@implementation RCAVSessionManagerTests

- (void)setUp
{
    [super setUp];
    // Set-up code here.
}

- (void)tearDown
{
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    [RCAVSessionManager sharedInstance];
    
    RCAVSessionManager* sessionMan1 = [RCAVSessionManager sharedInstance];
    RCAVSessionManager* sessionMan2 = [RCAVSessionManager sharedInstance];
    
    STAssertEqualObjects(sessionMan1, sessionMan2, @"Get instance failed to return the same instance");
}

- (void)testIsRunningAfterStarting
{
    RCAVSessionManager* sessionMan = [RCAVSessionManager sharedInstance];
    
    [sessionMan startSession];
    
    STAssertTrue([sessionMan isRunning], @"isRunning returned false while session was running");
}

- (void)testIsntRunningAfterEnding
{
    RCAVSessionManager* sessionMan = [RCAVSessionManager sharedInstance];
    
    [sessionMan startSession];
    [sessionMan endSession];
    
    STAssertFalse([sessionMan isRunning], @"Session running after endSession called");
}

@end


