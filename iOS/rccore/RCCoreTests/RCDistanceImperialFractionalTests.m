//
//  RCDistanceImperialFractionalTests.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceImperialFractionalTests.h"
#import "RCDistanceImperialFractional.h"

@implementation RCDistanceImperialFractionalTests

- (void) testInchFractions
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    // test generation of inch fractions
    
    testValue = 0.0625 * METERS_PER_INCH;
    expected = @"1/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.125 * METERS_PER_INCH;
    expected = @"1/8\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.1875 * METERS_PER_INCH;
    expected = @"3/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.25 * METERS_PER_INCH;
    expected = @"1/4\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.3125 * METERS_PER_INCH;
    expected = @"5/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.375 * METERS_PER_INCH;
    expected = @"3/8\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.4375 * METERS_PER_INCH;
    expected = @"7/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.5 * METERS_PER_INCH;
    expected = @"1/2\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.5625 * METERS_PER_INCH;
    expected = @"9/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.625 * METERS_PER_INCH;
    expected = @"5/8\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.6875 * METERS_PER_INCH;
    expected = @"11/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.75 * METERS_PER_INCH;
    expected = @"3/4\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.8125 * METERS_PER_INCH;
    expected = @"13/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.875 * METERS_PER_INCH;
    expected = @"7/8\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.9375 * METERS_PER_INCH;
    expected = @"15/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //edge cases
    testValue = 0.9999 * METERS_PER_INCH;
    expected = @"1\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1.0001 * METERS_PER_INCH;
    expected = @"1\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 11.9999 * METERS_PER_INCH;
    expected = @"12\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 12.0001 * METERS_PER_INCH;
    expected = @"12\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testFeetFractions
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 12.0625 * METERS_PER_INCH;
    expected = @"1' 1/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 12.9375 * METERS_PER_INCH;
    expected = @"1' 15/16\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //edge cases
    testValue = 11.9999 * METERS_PER_INCH;
    expected = @"1'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 12.0001 * METERS_PER_INCH;
    expected = @"1'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 23.9999 * METERS_PER_INCH;
    expected = @"2'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 24.0001 * METERS_PER_INCH;
    expected = @"2'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testYardFractions
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 35.9375 * METERS_PER_INCH;
//    expected = @"2' 11 15/16\"";
    expected = @"1yd";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 36.0625 * METERS_PER_INCH;
//    expected = @"1yd 1/16\"";
    expected = @"1yd";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 2 * METERS_PER_FOOT;
    expected = @"2'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 4 * METERS_PER_FOOT;
    expected = @"1yd 1'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 5 * METERS_PER_FOOT;
    expected = @"1yd 2'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //yards edge cases
    testValue = 2.9999 * METERS_PER_FOOT;
    expected = @"1yd";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 3.0001 * METERS_PER_FOOT;
    expected = @"1yd";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testMilesFractions
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 0.5 * METERS_PER_MILE;
    expected = @"2640'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = ((INCHES_PER_MILE / INCHES_PER_FOOT) - 1) * METERS_PER_FOOT;
    expected = @"5279'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 5281 * METERS_PER_FOOT;
    expected = @"1mi 1'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //miles edge cases
    testValue = .999999 * METERS_PER_MILE;
    expected = @"1mi";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1.0000001 * METERS_PER_MILE;
    expected = @"1mi";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testKnownExamples
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    //test some known examples
    testValue = 0.330;
    expected = @"1' 1\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1.19;
    expected = @"3' 10 7/8\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1.19;
    expected = @"1yd 11\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = .340;
    expected = @"13 3/8\"";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1761.454;
    expected = @"1mi 499'";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
}

- (void) testZeros
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 0.0;
    expected = @"0";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0;
    expected = @"0";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleFT] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0;
    expected = @"0";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleYD] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0;
    expected = @"0";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleMI] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //near zero
    testValue = 0.00079; //just under 1/32"
    expected = @"0";
    result = [[[RCDistanceImperialFractional alloc] initWithMeters:testValue withScale:UnitsScaleIN] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

@end
