//
//  TMDistanceFormatter.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMDistanceFormatter.h"
#import "TMMeasurement.h"

@implementation TMDistanceFormatter

typedef struct {
    uint8_t nominator;
    uint8_t denominator;
} Fraction;

+ (NSString*)formattedDistance:(NSNumber*)dist withUnits:(NSNumber*)units withScale:(NSNumber*)scale withFractional:(NSNumber*)fractional
{
    NSString *unitsSymbol;
    NSNumber *convertedDist;
    
    if(units.integerValue == UNITS_PREF_METRIC)
    {
        if(scale.integerValue == UNITSSCALE_PREF_CM)
        {
            unitsSymbol = @"cm";
            convertedDist = [NSNumber numberWithFloat:dist.floatValue * 1000]; //convert to cm
            return [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist.floatValue, unitsSymbol];
        }
        else if(scale.integerValue == UNITSSCALE_PREF_KM)
        {
            unitsSymbol = @"km";
            convertedDist = [NSNumber numberWithFloat:dist.floatValue / 1000]; //convert to km
            return [NSString localizedStringWithFormat:@"%0.5f%@", convertedDist.floatValue, unitsSymbol];
        }
        else
        {
            unitsSymbol = @"m";
            return [NSString localizedStringWithFormat:@"%0.3f%@", dist.floatValue, unitsSymbol];
        }
    }
    else
    { 
        convertedDist = [NSNumber numberWithFloat:dist.floatValue * 39.3700787]; //convert to inches
        
        if(scale.integerValue == UNITSSCALE_PREF_FT)
        {
            convertedDist = [NSNumber numberWithFloat:convertedDist.floatValue / 12]; //convert to feet
            unitsSymbol = @"\'";
            return [NSString localizedStringWithFormat:@"%0.2f%@", convertedDist.floatValue, unitsSymbol];
        }
        else if(scale.integerValue == UNITSSCALE_PREF_YD)
        {
            convertedDist = [NSNumber numberWithFloat:convertedDist.floatValue / 36]; //convert to yards
            unitsSymbol = @"yd";
            return [NSString localizedStringWithFormat:@"%0.3f%@", convertedDist.floatValue, unitsSymbol];
        }
        else if(scale.integerValue == UNITSSCALE_PREF_MI)
        {
            convertedDist = [NSNumber numberWithFloat:convertedDist.floatValue / 5280]; //convert to miles
            unitsSymbol = @"mi";
            return [NSString localizedStringWithFormat:@"%0.5f%@", convertedDist.floatValue, unitsSymbol];
        }
        else
        {
            unitsSymbol = @"\"";
            if(fractional.boolValue)
            {
                Fraction fract = [self getInchFraction:convertedDist];
                return [NSString localizedStringWithFormat:@"%0.0f %u/%u%@", convertedDist.floatValue, fract.nominator, fract.denominator, unitsSymbol];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist.floatValue, unitsSymbol];
            }
        }
    }
}

+ (NSString*)formattedDistance:(NSNumber *)dist withMeasurement:(TMMeasurement *)measurement
{
    if(measurement.units.integerValue == UNITS_PREF_METRIC)
    {
        return [self formattedDistance:dist withUnits:measurement.units withScale:measurement.unitsScaleMetric withFractional:measurement.fractional];
    }
    else
    {
        return [self formattedDistance:dist withUnits:measurement.units withScale:measurement.unitsScaleImperial withFractional:measurement.fractional];
    }
}

+ (Fraction)getInchFraction:(NSNumber*)inches
{
    uint32_t wholeInches = floor(inches.floatValue);
    float remainder = inches.floatValue - wholeInches;
    
    uint8_t sixteenths = round(remainder * 16);
    
    int gcd = [self gcdForNumber1:sixteenths andNumber2:16];
    
    uint8_t nominator = sixteenths / gcd;
    uint8_t denominator = 16 / gcd;
    
    return (Fraction) { .nominator = nominator, .denominator = denominator };
}

+ (int)gcdForNumber1:(int) m andNumber2:(int) n
{
    if(!(m && n)) return 1;
    
    while( m!= n) // execute loop until m == n
    {
        if( m > n)
            m= m - n; // large - small , store the results in large variable<br>
        else
            n= n - m;
    }
    return ( m); // m or n is GCD
}

@end
