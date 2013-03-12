//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "RCDistanceFormatter.h"

@implementation RCDistanceFormatter

+ (NSString*)getFormattedDistance:(float)meters withUnits:(Units)units withScale:(UnitsScale)scale withFractional:(BOOL)fractional
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
        convertedDist = meters * INCHES_PER_METER; //convert to inches
        
        if(scale == UnitsScaleFT)
        {
            if(fractional)
            {
                return [self getFormattedFractionalFeet:convertedDist];
            }
            else
            {
                convertedDist = convertedDist / INCHES_PER_FOOT;
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
                convertedDist = convertedDist / INCHES_PER_YARD;
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
                convertedDist = convertedDist / INCHES_PER_MILE;
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
//+ (NSString*)getFormattedDistance:(NSNumber *)meters withMeasurement:(TMMeasurement *)measurement
//{
//    if((Units)measurement.units.intValue == UnitsMetric)
//    {
//        return [self getFormattedDistance:meters.floatValue withUnits:measurement.units.intValue withScale:measurement.unitsScaleMetric.intValue withFractional:measurement.fractional.boolValue];
//    }
//    else
//    {
//        return [self getFormattedDistance:meters.floatValue withUnits:measurement.units.intValue withScale:measurement.unitsScaleImperial.intValue withFractional:measurement.fractional.boolValue];
//    }
//}

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
        if (INCHES_PER_FOOT - remainingInches < 1)
        {
            Fraction fract = [self getInchFraction:remainingInches];
            if (fract.nominator / fract.denominator == 1) return [NSString stringWithFormat:@"%0.0fmi", (floor(miles) + 1)];
        }
        
        NSString *fractionalFeet = [self getFormattedFractionalFeet:remainingInches];
        if(result.length > 0 && fractionalFeet.length) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole yards
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
        if (INCHES_PER_FOOT - remainingInches < 1)
        {
            Fraction fract = [self getInchFraction:remainingInches];
            if (fract.nominator / fract.denominator == 1) return [NSString stringWithFormat:@"%0.0fyd", (floor(yards) + 1)];
        }
        
        NSString *fractionalFeet = [self getFormattedFractionalFeet:remainingInches];
        if(result.length && fractionalFeet.length) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole yards
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
        if (INCHES_PER_FOOT - remainingInches < 1)
        {
            Fraction fract = [self getInchFraction:remainingInches];
            if (fract.nominator / fract.denominator == 1) return [NSString stringWithFormat:@"%0.0f'", (floor(feet) + 1)];
        }
        
        NSString *fractionalInches = [self getFormattedFractionalInches:remainingInches];
        if(result.length && fractionalInches.length) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole feet
        result = [NSString stringWithFormat:@"%@%@", result, fractionalInches];
    }

    return result;
}

+ (NSString*)getFormattedFractionalInches:(float)inches
{
    NSString *result = @"";
    
    if (inches <= 0) return @"0\"";
   
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
    
    if(fract.nominator && (float)fract.denominator / (float)fract.nominator > 1)
    {
        if(result.length > 0) result = [NSString stringWithFormat:@"%@ ", result]; //add space if there are any whole inches
        result = [NSString stringWithFormat:@"%@%u/%u", result, fract.nominator, fract.denominator];
    }
    
    if (result.length) result = [NSString stringWithFormat:@"%@\"", result]; //add " inches symbol
    return result;
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

/** @returns Greatest common denominator for two numbers */
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

+ (UnitsScale)autoSelectUnitsScale:(float)meters withUnits:(Units)units
{
    if (units == UnitsImperial)
    {
        float inches = meters * INCHES_PER_METER;
        
        if (inches < INCHES_PER_FOOT) return UnitsScaleIN;
        if (inches >= INCHES_PER_MILE) return UnitsScaleMI;
        return UnitsScaleFT; //default
    }
    else
    {
        if (meters < 1) return UnitsScaleCM;
        if (meters >= 1000) return UnitsScaleKM;
        return UnitsScaleM; //default
    }
}

@end
