//
//  RCSensorFusion+RCSensorFusionMock.m
//  RCCore
//
//  Created by Ben Hirashima on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCSensorFusion+RCSensorFusionMock.h"
#import "NSObject+SupersequentImplementation.h"
#import "OCMock.h"

@implementation RCSensorFusion (RCSensorFusionMock)

static id mockSensorFusion;

+ (RCAVSessionManager*) sharedInstance
{
    if (mockSensorFusion)
        return mockSensorFusion;
    else
        return invokeSupersequentNoParameters();
}

+ (void) setupMock
{
    mockSensorFusion = [OCMockObject mockForClass:[RCSensorFusion class]];
}

+ (void) setupNiceMock
{
    mockSensorFusion = [OCMockObject niceMockForClass:[RCSensorFusion class]];
}

+ (void) tearDownMock
{
    mockSensorFusion = nil;
}

@end
