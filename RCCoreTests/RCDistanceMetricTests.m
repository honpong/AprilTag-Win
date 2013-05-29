//
//  RCDistanceMetricTests.m
//  RCCore
//
//  Created by Ben Hirashima on 5/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceMetricTests.h"
#import "RCDistanceMetric.h"

@implementation RCDistanceMetricTests

- (void) testMeters
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    //test that we still get meters when meters < 1
    testValue = 0.5;
    expected = @"0.50m";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1;
    expected = @"1.00m";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1000;
    expected = @"1,000.00m";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testCentimeters
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 0.5;
    expected = @"50.0cm";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleCM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1;
    expected = @"100.0cm";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleCM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //test that we still get centimeters when > 1 meter
    testValue = 1000;
    expected = @"100,000.0cm";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleCM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testKilometers
{
    NSString *result;
    NSString *expected;
    float testValue;
    
    testValue = 1;
    expected = @"0.001km";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleKM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 1000;
    expected = @"1.000km";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleKM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    //test that we still get centimeters when > 1 meter
    testValue = 10000;
    expected = @"10.000km";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleKM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

- (void) testZeros
{
    NSString *result;
    NSString *expected;
    float testValue;
   
    testValue = 0.0;
    expected = @"0.0cm";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleCM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0;
    expected = @"0.00m";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
    
    testValue = 0.0;
    expected = @"0.000km";
    result = [[[RCDistanceMetric alloc] initWithMeters:testValue withScale:UnitsScaleKM] getString];
    STAssertTrue([result isEqualToString:expected], [NSString stringWithFormat:@"%f should be [%@], got [%@]", testValue, expected, result]);
}

@end
