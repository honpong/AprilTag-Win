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

- (void) testInchFractions
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
}

- (void) testInchDecimals
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 11 * METERS_PER_INCH;
    expected = @"11.0\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 12 * METERS_PER_INCH;
    expected = @"12.0\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //test that we still get inches when inches > 12 and units scale = UnitsScaleIN
    testValue = 13 * METERS_PER_INCH;
    expected = @"13.0\"";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleIN withFractional:NO];
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
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
        
    testValue = 1 * METERS_PER_FOOT;
    expected = @"1.00\'";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //test that we still get feet when feet > 1 yard
    testValue = 4 * METERS_PER_FOOT;
    expected = @"4.00\'";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleFT withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testYardDecimals
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    //test that we still get yards when yards < 1
    testValue = 0.5 * METERS_PER_YARD;
    expected = @"0.500yd";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleYD withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1 * METERS_PER_YARD;
    expected = @"1.000yd";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleYD withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //test that we still get yards when yards > 1 mile
    testValue = 1761 * METERS_PER_YARD;
    expected = @"1,761.000yd";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleYD withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testMileDecimals
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    //test that we still get miles when miles < 1
    testValue = 0.5 * INCHES_PER_MILE * METERS_PER_INCH;
    expected = @"0.50000mi";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleMI withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1 * INCHES_PER_MILE * METERS_PER_INCH;
    expected = @"1.00000mi";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsImperial withScale:UnitsScaleMI withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testMeters
{
    NSString *result;
    NSString *expected;
    float testValue;

    //test that we still get meters when meters < 1
    testValue = 0.5;
    expected = @"0.50m";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleM withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);

    testValue = 1;
    expected = @"1.00m";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleM withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);

    testValue = 1000;
    expected = @"1,000.00m";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleM withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testCentimeters
{
    NSString *result;
    NSString *expected;
    float testValue;

    testValue = 0.5;
    expected = @"50.0cm";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleCM withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);

    testValue = 1;
    expected = @"100.0cm";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleCM withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);

    //test that we still get centimeters when > 1 meter
    testValue = 1000;
    expected = @"100,000.0cm";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleCM withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testKilometers
{
    NSString *result;
    NSString *expected;
    float testValue;

    testValue = 1;
    expected = @"0.001km";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleKM withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);

    testValue = 1000;
    expected = @"1.000km";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleKM withFractional:NO];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);

    //test that we still get centimeters when > 1 meter
    testValue = 10000;
    expected = @"10.000km";
    result = [RCDistanceFormatter getFormattedDistance:testValue withUnits:UnitsMetric withScale:UnitsScaleKM withFractional:NO];
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

- (void) testAutoSelectUnitsScale
{
    float testValue;
    Units testUnits;
    UnitsScale expected;
    UnitsScale result;
    
    testUnits = UnitsMetric;
    
    testValue = 0;
    expected = UnitsScaleCM;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 0.999f;
    expected = UnitsScaleCM;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 1.0f;
    expected = UnitsScaleM;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 1.001f;
    expected = UnitsScaleM;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 999.9f;
    expected = UnitsScaleM;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 1000.0f;
    expected = UnitsScaleKM;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 1000.001f;
    expected = UnitsScaleKM;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testUnits = UnitsImperial;
       
    testValue = 0;
    expected = UnitsScaleIN;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 11.999f * METERS_PER_INCH;
    expected = UnitsScaleIN;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 12.0f * METERS_PER_INCH;
    expected = UnitsScaleFT;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = 12.001f * METERS_PER_INCH;
    expected = UnitsScaleFT;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = (INCHES_PER_MILE - 1) * METERS_PER_INCH;
    expected = UnitsScaleFT;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = INCHES_PER_MILE * METERS_PER_INCH;
    expected = UnitsScaleMI;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
    
    testValue = (INCHES_PER_MILE + 1) * METERS_PER_INCH;
    expected = UnitsScaleMI;
    result = [RCDistanceFormatter autoSelectUnitsScale:testValue withUnits:testUnits];
    STAssertEquals(result, expected, [NSString stringWithFormat:@"Expected %u, from %f", expected, testValue]);
}

@end
