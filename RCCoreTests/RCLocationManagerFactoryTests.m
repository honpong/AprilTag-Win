//
//  RCLocationManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCLocationManagerFactoryTests.h"
#import "RCLocationManagerFactory.h"
#import <OCMock.h>

@implementation RCLocationManagerFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    [RCLocationManagerFactory setInstance:nil];
    
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    id<RCLocationManager> locMan1 = [RCLocationManagerFactory getInstance];
    id<RCLocationManager> locMan2 = [RCLocationManagerFactory getInstance];
    
    STAssertEqualObjects(locMan1, locMan2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    id locMan1 = [OCMockObject mockForProtocol:@protocol(RCLocationManager)];

    [RCLocationManagerFactory setInstance:locMan1];
    
    id locMan2 = [RCLocationManagerFactory getInstance];
    
    STAssertEqualObjects(locMan1, locMan2, @"Get instance failed to return the same instance after set instance was called");
}

- (void)testStart
{
    id<RCLocationManager> locMan = [RCLocationManagerFactory getInstance];
    
    id mockCLLocMan = [OCMockObject mockForClass:[CLLocationManager class]];
    [[mockCLLocMan expect] setDesiredAccuracy:kCLLocationAccuracyBest];
    [[mockCLLocMan expect] setDistanceFilter:500];
    [[mockCLLocMan expect] setDelegate:locMan];
    [[mockCLLocMan expect] startUpdatingLocation];
    
    [locMan startLocationUpdates:mockCLLocMan];
    
    [mockCLLocMan verify];
}

- (void)testStop
{
    id<RCLocationManager> locMan = [RCLocationManagerFactory getInstance];
    
    id mockCLLocMan = [OCMockObject niceMockForClass:[CLLocationManager class]];
        
    [locMan startLocationUpdates:mockCLLocMan];
    
    [[mockCLLocMan expect] stopUpdatingLocation];
    
    [locMan stopLocationUpdates];
    
    [mockCLLocMan verify];
}

@end
