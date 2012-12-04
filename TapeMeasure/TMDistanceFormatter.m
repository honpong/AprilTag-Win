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
    float convertedDist;
    
    float distance = dist.floatValue;
    
    if(units.integerValue == UNITS_PREF_METRIC)
    {
        if(scale.integerValue == UNITSSCALE_PREF_CM)
        {
            unitsSymbol = @"cm";
            convertedDist = distance * 1000; //convert to cm
            return [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist, unitsSymbol];
        }
        else if(scale.integerValue == UNITSSCALE_PREF_KM)
        {
            unitsSymbol = @"km";
            convertedDist = distance / 1000; //convert to km
            return [NSString localizedStringWithFormat:@"%0.5f%@", convertedDist, unitsSymbol];
        }
        else
        {
            unitsSymbol = @"m";
            return [NSString localizedStringWithFormat:@"%0.3f%@", distance, unitsSymbol];
        }
    }
    else
    { 
        convertedDist = distance * 39.3700787; //convert to inches
        
        if(scale.integerValue == UNITSSCALE_PREF_FT)
        {
            if(fractional.boolValue)
            {
                return [self getFormattedFractionalFeet:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.2f\'", convertedDist];
            }
        }
        else if(scale.integerValue == UNITSSCALE_PREF_YD)
        {
            if(fractional.boolValue)
            {
                return [self getFormattedFractionalYards:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.3fyd", convertedDist];
            }
        }
        else if(scale.integerValue == UNITSSCALE_PREF_MI)
        {
            if(fractional.boolValue)
            {
                return [self getFormattedFractionalMiles:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.5fmi", convertedDist];
            }
        }
        else
        {
            if(fractional.boolValue)
            {
                return [self getFormattedFractionalInches:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.1f\"", convertedDist];
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

+ (NSString*)getFormattedFractionalMiles:(float)inches
{
    NSString *result = @"";
    
    float miles = inches / 63360; 
    
    if(miles >= 1)
    {
        result = [NSString localizedStringWithFormat:@"%0.0fmi", floor(miles)];
    }
    
    float remainingInches = inches - (floor(miles) * 63360);
    
    if(remainingInches > 0)
    {
        NSString *fractionalFeet = [self getFormattedFractionalFeet:remainingInches];
        if(result.length > 0) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole yards
        result =  [NSString stringWithFormat:@"%@%@", result, fractionalFeet];
    }
    
    return result;
}

+ (NSString*)getFormattedFractionalYards:(float)inches
{
    NSString *result = @"";
    
    float yards = inches / 36;
    
    if(yards >= 1)
    {
        result = [NSString localizedStringWithFormat:@"%0.0fyd", floor(yards)];
    }
    
    float remainingInches = inches - (floor(yards) * 36);
    
    if(remainingInches > 0)
    {
        NSString *fractionalFeet = [self getFormattedFractionalFeet:remainingInches];
        if(result.length > 0) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole yards
        result =  [NSString stringWithFormat:@"%@%@", result, fractionalFeet];
    }

    return result;
}

+ (NSString*)getFormattedFractionalFeet:(float)inches
{
    NSString *result = @"";
    
    float feet = inches / 12;
    
    if(feet >= 1)
    {
        result = [NSString localizedStringWithFormat:@"%0.0f'", floor(feet)];
    }
    
    float remainingInches = inches - (floor(feet) * 12);
    
    if(remainingInches > 0)
    {
        NSString *fractionalInches = [self getFormattedFractionalInches:remainingInches];
        if(result.length > 0) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole feet
        result = [NSString stringWithFormat:@"%@%@", result, fractionalInches];
    }

    return result;
}

+ (NSString*)getFormattedFractionalInches:(float)inches
{
    NSString *result = @"";

    if(inches >= 1)
    {
        result = [NSString localizedStringWithFormat:@"%0.0f", floor(inches)];
    }
   
    Fraction fract = [self getInchFraction:inches];
    
    if(fract.nominator > 0)
    {
        if(result.length > 0) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole inches
        result = [NSString localizedStringWithFormat:@"%@%u/%u\"", result, fract.nominator, fract.denominator];
    }
    
    return result;
}

+ (Fraction)getInchFraction:(float)inches
{
    uint32_t wholeInches = floor(inches);
    float remainder = inches - wholeInches;
    
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
