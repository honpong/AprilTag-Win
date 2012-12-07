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

const float METERS_PER_INCH = 0.0254;
const float INCHES_PER_METER = 39.3700787;
const int INCHES_PER_FOOT = 12;
const int INCHES_PER_YARD = 36;
const int INCHES_PER_MILE = 63360;

+ (NSString*)formattedDistance:(float)meters withUnits:(Units)units withScale:(UnitsScale)scale withFractional:(BOOL)fractional
{
    NSString *unitsSymbol;
    float convertedDist;
    
    if(units == UnitsMetric)
    {
        if(scale == UnitsScaleCM)
        {
            unitsSymbol = @"cm";
            convertedDist = meters * 1000; //convert to cm
            return [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist, unitsSymbol];
        }
        else if(scale == UnitsScaleKM)
        {
            unitsSymbol = @"km";
            convertedDist = meters / 1000; //convert to km
            return [NSString localizedStringWithFormat:@"%0.5f%@", convertedDist, unitsSymbol];
        }
        else if(scale == UnitsScaleM)
        {
            unitsSymbol = @"m";
            return [NSString localizedStringWithFormat:@"%0.3f%@", meters, unitsSymbol];
        }
        else
        {
            //TODO: throw NSError
            return @"ERROR";
        }
    }
    else if(units == UnitsImperial)
    { 
        convertedDist = meters * 39.3700787; //convert to inches
        
        if(scale == UnitsScaleFT)
        {
            if(fractional)
            {
                return [self getFormattedFractionalFeet:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.2f\'", convertedDist];
            }
        }
        else if(scale == UnitsScaleYD)
        {
            if(fractional)
            {
                return [self getFormattedFractionalYards:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.3fyd", convertedDist];
            }
        }
        else if(scale == UnitsScaleMI)
        {
            if(fractional)
            {
                return [self getFormattedFractionalMiles:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.5fmi", convertedDist];
            }
        }
        else if(scale == UnitsScaleIN)
        {
            if(fractional)
            {
                return [self getFormattedFractionalInches:convertedDist];
            }
            else
            {
                return [NSString localizedStringWithFormat:@"%0.1f\"", convertedDist];
            }
        }
        else
        {
            //TODO: throw NSError
            return @"ERROR";
        }
    }
    else
    {
        //TODO: throw NSError
        return @"ERROR"; 
    }
}

//convenience method
+ (NSString*)formattedDistance:(NSNumber *)meters withMeasurement:(TMMeasurement *)measurement
{
    if((Units)measurement.units.intValue == UnitsMetric)
    {
        return [self formattedDistance:meters.floatValue withUnits:measurement.units.intValue withScale:measurement.unitsScaleMetric.intValue withFractional:measurement.fractional.boolValue];
    }
    else
    {
        return [self formattedDistance:meters.floatValue withUnits:measurement.units.intValue withScale:measurement.unitsScaleImperial.intValue withFractional:measurement.fractional.boolValue];
    }
}

+ (NSString*)getFormattedFractionalMiles:(float)inches
{
    NSString *result = @"";
    
    float miles = inches / INCHES_PER_MILE;
    
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
    
    float yards = inches / INCHES_PER_YARD;
    
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
    
    float feet = inches / INCHES_PER_FOOT;
    
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
   
    Fraction fract = [self getInchFraction:inches];
    
    if(fract.nominator / fract.denominator == 1)
    {
        result = [NSString localizedStringWithFormat:@"%0.0f", ceil(inches)];
    }
    else
    {
        if(inches >= 1)
        {
            result = [NSString localizedStringWithFormat:@"%0.0f", floor(inches)];
        }
    }
    
    if((float)fract.denominator / (float)fract.nominator > 1)
    {
        if(result.length > 0) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole inches
        result = [NSString stringWithFormat:@"%@%u/%u", result, fract.nominator, fract.denominator];
//        result = [NSString localizedStringWithFormat:@"%@%u\u2044%u\"", result, fract.nominator, fract.denominator]; //special slash for fractions
    }
    
    return [NSString stringWithFormat:@"%@\"", result]; //add " inches symbol
}

+ (Fraction)getInchFraction:(float)inches
{
    int wholeInches = floor(inches);
    float remainder = inches - wholeInches;
    
    int sixteenths = roundf(remainder * 16);
    
    int gcd = [self gcdForNumber1:sixteenths andNumber2:16];
    
    int nominator = sixteenths / gcd;
    int denominator = 16 / gcd;
    
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
