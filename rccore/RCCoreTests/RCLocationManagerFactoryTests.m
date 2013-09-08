//
//  RCLocationManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCLocationManagerFactoryTests.h"
#import "RCLocationManager.h"
#import "OCMock.h"

/////////////////////// CLLocationManager (Mock) ///////////////////////

@interface CLLocationManager (Stub)
+ (BOOL) headingAvailable;
@end

@implementation CLLocationManager (Stub)
+ (BOOL) headingAvailable
{
    return YES;
}
@end

/////////////////////// RCLocationManagerFactoryTests ///////////////////////

@implementation RCLocationManagerFactoryTests

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
    RCLocationManager* locMan1 = [RCLocationManager sharedInstance];
    RCLocationManager* locMan2 = [RCLocationManager sharedInstance];
    
    STAssertEqualObjects(locMan1, locMan2, @"Get instance failed to return the same instance");
}

- (void)testStartAndStopAndManualStop
{
    RCLocationManager* locMan = [RCLocationManager new];
    
    id mockCLLocMan = [OCMockObject niceMockForClass:[CLLocationManager class]];
    [[mockCLLocMan expect] setDelegate:[OCMArg any]];
    [[mockCLLocMan expect] startUpdatingLocation];
    
    [locMan startLocationUpdates:mockCLLocMan];
    
    [mockCLLocMan verify];
    STAssertTrue([locMan isUpdatingLocation], @"isUpdatingLocation should be true after start");
    
    [[mockCLLocMan expect] stopUpdatingLocation];
    [locMan stopLocationUpdates];
    
    [mockCLLocMan verify];
    STAssertFalse([locMan isUpdatingLocation], @"isUpdatingLocation should be false after stop");
}

- (void)testStartAndAutomaticStop
{
    RCLocationManager* locMan = [RCLocationManager new];
    
    id mockCLLocMan = [OCMockObject niceMockForClass:[CLLocationManager class]];
    [[mockCLLocMan expect] setDelegate:[OCMArg any]];
    [[mockCLLocMan expect] startUpdatingLocation];
    
    [locMan startLocationUpdates:mockCLLocMan];
    
    [mockCLLocMan verify];
    STAssertTrue([locMan isUpdatingLocation], @"isUpdatingLocation should be true after start");
    
    [[mockCLLocMan expect] stopUpdatingLocation];
    
    id mockCLLocation = [OCMockObject niceMockForClass:[CLLocation class]];
    [[[mockCLLocation stub] andReturn:[NSDate date]] timestamp];
    [[[mockCLLocation stub] andReturnValue:OCMOCK_VALUE((CLLocationAccuracy){65})] horizontalAccuracy];
    [locMan locationManager:mockCLLocMan didUpdateLocations:[NSArray arrayWithObject:mockCLLocation]];
    
    [mockCLLocMan verify];
    STAssertFalse([locMan isUpdatingLocation], @"isUpdatingLocation should be false after stop");
}

- (void)testHeadingStartAndStop
{
    RCLocationManager* locMan = [RCLocationManager new];
    
    id mockCLLocMan = [OCMockObject niceMockForClass:[CLLocationManager class]];
    [[mockCLLocMan expect] setDelegate:[OCMArg any]];
    [[mockCLLocMan expect] startUpdatingLocation];
    
    [locMan startLocationUpdates:mockCLLocMan];
    [mockCLLocMan verify];
    
    [[mockCLLocMan expect] setDelegate:[OCMArg any]];
    [[mockCLLocMan expect] startUpdatingHeading];
    
    [locMan startHeadingUpdates];
    [mockCLLocMan verify];
    
    STAssertTrue([locMan isUpdatingLocation], @"isUpdatingLocation should be true after start");
    
    // make sure an incoming location doesn't stop heading updates
    id mockCLLocation = [OCMockObject niceMockForClass:[CLLocation class]];
    [[[mockCLLocation stub] andReturn:[NSDate date]] timestamp];
    [[[mockCLLocation stub] andReturnValue:OCMOCK_VALUE((CLLocationAccuracy){65})] horizontalAccuracy];
    [locMan locationManager:mockCLLocMan didUpdateLocations:[NSArray arrayWithObject:mockCLLocation]];
    
    STAssertTrue([locMan isUpdatingLocation], @"isUpdatingLocation should still be true after receiving location, because heading updates are on");
    
    [[mockCLLocMan expect] stopUpdatingHeading];
    [locMan stopHeadingUpdates];
    [[mockCLLocMan expect] stopUpdatingLocation];
    [locMan stopLocationUpdates];
    
    [mockCLLocMan verify];
    STAssertFalse([locMan isUpdatingLocation], @"isUpdatingLocation should be false after stop");
}

@end
