//
//  RCAVSessionManager+RCAVSessionManagerMock.m
//  RCCore
//
//  Created by Ben Hirashima on 6/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCAVSessionManager+RCAVSessionManagerMock.h"
#import "OCMock.h"
#import "NSObject+SupersequentImplementation.h"

@implementation RCAVSessionManager (RCAVSessionManagerMock)

static id mockAVSessionManager;

+ (RCAVSessionManager*) sharedInstance
{
    if (mockAVSessionManager)
        return mockAVSessionManager;
    else
        return invokeSupersequentNoParameters();
}

+ (void) setupMock
{
    mockAVSessionManager = [OCMockObject mockForClass:[RCAVSessionManager class]];
}

+ (void) setupNiceMock
{
    mockAVSessionManager = [OCMockObject niceMockForClass:[RCAVSessionManager class]];
}

+ (void) tearDownMock
{
    mockAVSessionManager = nil;
}

@end
