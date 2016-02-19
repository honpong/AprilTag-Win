//
//  RCDistanceImperialTests.m
//  RCCore
//
//  Created by Ben Hirashima on 5/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceImperialTests.h"
#import "RCDistanceImperial.h"

@implementation RCDistanceImperialTests

- (void) testInchDecimals
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 11 * METERS_PER_INCH;
    expected = @"11.0\"";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 12 * METERS_PER_INCH;
    expected = @"12.0\"";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //test that we still get inches when inches > 12 and units scale = UnitsScaleIN
    testValue = 13 * METERS_PER_INCH;
    expected = @"13.0\"";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testFootDecimals
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    //test that we still get feet when feet < 1
    testValue = 0.5 * METERS_PER_FOOT;
    expected = @"0.50\'";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1 * METERS_PER_FOOT;
    expected = @"1.00\'";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //test that we still get feet when feet > 1 yard
    testValue = 4 * METERS_PER_FOOT;
    expected = @"4.00\'";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testYardDecimals
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    //test that we still get yards when yards < 1
    testValue = 0.5 * METERS_PER_YARD;
    expected = @"0.50yd";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1 * METERS_PER_YARD;
    expected = @"1.00yd";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //test that we still get yards when yards > 1 mile
    testValue = 1761 * METERS_PER_YARD;
    expected = @"1,761.00yd";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testMileDecimals
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    //test that we still get miles when miles < 1
    testValue = 0.5 * INCHES_PER_MILE * METERS_PER_INCH;
    expected = @"0.500mi";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1 * INCHES_PER_MILE * METERS_PER_INCH;
    expected = @"1.000mi";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testZeros
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 0.0;
    expected = @"0.0\"";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0;
    expected = @"0.00'";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0;
    expected = @"0.00yd";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0;
    expected = @"0.000mi";
    result = [[[RCDistanceImperial alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

@end
