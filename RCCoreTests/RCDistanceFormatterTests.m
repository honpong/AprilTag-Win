//
//  RCDistanceFormatterTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceFormatterTests.h"
#import "RCDistanceFormatter.h"

@implementation RCDistanceFormatterTests

- (void)testDistanceFormatter
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    // test generation of inch fractions
    testValue = 0.0;
    expected = @"0\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0625 * METERS_PER_INCH;
    expected = @"1/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.125 * METERS_PER_INCH;
    expected = @"1/8\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.1875 * METERS_PER_INCH;
    expected = @"3/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.25 * METERS_PER_INCH;
    expected = @"1/4\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.3125 * METERS_PER_INCH;
    expected = @"5/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.375 * METERS_PER_INCH;
    expected = @"3/8\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.4375 * METERS_PER_INCH;
    expected = @"7/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.5 * METERS_PER_INCH;
    expected = @"1/2\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.5625 * METERS_PER_INCH;
    expected = @"9/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.625 * METERS_PER_INCH;
    expected = @"5/8\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.6875 * METERS_PER_INCH;
    expected = @"11/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.75 * METERS_PER_INCH;
    expected = @"3/4\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.8125 * METERS_PER_INCH;
    expected = @"13/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.875 * METERS_PER_INCH;
    expected = @"7/8\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.9375 * METERS_PER_INCH;
    expected = @"15/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    
    //feet edge cases
    testValue = 0.9999 * METERS_PER_INCH;
    expected = @"1\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1.0001 * METERS_PER_INCH;
    expected = @"1\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 11.9999 * METERS_PER_INCH;
    expected = @"12\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 12.0001 * METERS_PER_INCH;
    expected = @"12\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 11.9999 * METERS_PER_INCH;
    expected = @"1'";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 12.0001 * METERS_PER_INCH;
    expected = @"1'";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 23.9999 * METERS_PER_INCH;
    expected = @"2'";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 24.0001 * METERS_PER_INCH;
    expected = @"2'";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    
    //yards edge cases
    testValue = 2.9999 * METERS_PER_FOOT;
    expected = @"1yd";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleYD withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 3.0001 * METERS_PER_FOOT;
    expected = @"1yd";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleYD withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    
    //miles edge cases
    testValue = 1759.9999 * METERS_PER_YARD;
    expected = @"1mi";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleMI withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1760.0001 * METERS_PER_YARD;
    expected = @"1mi";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleMI withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    
    //test some known examples
    testValue = 0.330;
    expected = @"1' 1\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1.19;
    expected = @"3' 10 7/8\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1.19;
    expected = @"1yd 10 7/8\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleYD withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = .340;
    expected = @"13 3/8\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1761.454;
    expected = @"1mi 499' 9/16\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleMI withFractional:YES];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
}

@end
