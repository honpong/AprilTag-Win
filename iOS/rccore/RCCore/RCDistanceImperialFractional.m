//
//  RCDistanceImperialFractional.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceImperialFractional.h"
#import "RCDebugLog.h"

@implementation RCDistanceImperialFractional
{
    NSString* stringRep;
    NSString* stringWithoutFractionOrUnitsSymbol;
    double _miles;
    double _yards;
    double _feet;
    double _inches;
}
@synthesize meters, scale, wholeMiles, wholeYards, wholeFeet, wholeInches, fraction;

+ (RCDistanceImperialFractional*) distWithMeters:(float)meters_ withScale:(UnitsScale)unitsScale
{
    return [[RCDistanceImperialFractional alloc] initWithMeters:meters_ withScale:unitsScale];
}

+ (RCDistanceImperialFractional*) distWithMiles:(int)miles withYards:(int)yards withFeet:(int)feet withInches:(int)inches withNominator:(int)nom withDenominator:(int)denom
{
    RCDistanceImperialFractional* dist = [[RCDistanceImperialFractional alloc] initWithMiles:miles withYards:yards withFeet:feet withInches:inches withNominator:nom withDenominator:denom withScale:UnitsScaleIN];
    return dist;
}

+ (RCDistanceImperialFractional*) distWithMiles:(int)miles withYards:(int)yards withFeet:(int)feet withInches:(int)inches withNominator:(int)nom withDenominator:(int)denom withScale:(UnitsScale)unitsScale
{
    RCDistanceImperialFractional* dist = [[RCDistanceImperialFractional alloc] initWithMiles:miles withYards:yards withFeet:feet withInches:inches withNominator:nom withDenominator:denom withScale:unitsScale];
    return dist;
}

- (id) initWithMeters:(float)meters_ withScale:(UnitsScale)unitsScale
{
    if(self = [super init])
    {
        _inches = _feet = _yards = _miles = 0;
        wholeMiles = wholeYards = wholeFeet = wholeInches = 0;
        meters = fabsf(meters_);
        scale = unitsScale;
        
        _inches = meters * INCHES_PER_METER; //convert to inches
        
        if(scale == UnitsScaleFT)
        {
            [self calculateFeet:_inches];
        }
        else if(scale == UnitsScaleYD)
        {
            [self calculateYards:_inches];
        }
        else if(scale == UnitsScaleMI)
        {
            [self calculateMiles:_inches];
        }
        else if(scale == UnitsScaleIN)
        {
            [self calculateInches:_inches];
        }
        else
        {
            LOGME
            DLog(@"Unrecognized units scale");
        }
    }
    return self;
}

- (id) initWithMiles:(int)miles withYards:(int)yards withFeet:(int)feet withInches:(int)inches withNominator:(int)nom withDenominator:(int)denom withScale:(UnitsScale)unitsScale
{
    float meters_ = miles * METERS_PER_MILE + yards * METERS_PER_YARD + feet * METERS_PER_FOOT + inches * METERS_PER_INCH + ((float)nom/denom * METERS_PER_INCH);
    return [[RCDistanceImperialFractional alloc] initWithMeters:meters_ withScale:unitsScale];
}

- (void) calculateMiles:(double)inches
{
    _miles = inches / INCHES_PER_MILE;
    wholeMiles = floor(_miles);
    double remainingInches = inches - (wholeMiles * INCHES_PER_MILE);
    [self calculateFeet:remainingInches]; //skip to feet
}

- (void) calculateYards:(double)inches
{
    _yards = inches / INCHES_PER_YARD;
    wholeYards = floor(_yards);
    double remainingInches = inches - (wholeYards * INCHES_PER_YARD);
    [self calculateFeet:remainingInches];
}

- (void) calculateFeet:(double)inches
{
    _feet = inches / INCHES_PER_FOOT;
    wholeFeet = floor(_feet);
    double remainingInches = inches - (wholeFeet * INCHES_PER_FOOT);
    [self calculateInches:remainingInches];
}

- (void) calculateInches:(double)inches
{
    wholeInches = floor(inches);
    double remainingInchFraction = inches - wholeInches;
    if (remainingInchFraction >= 1 - THIRTYSECOND_INCH)
    {
        wholeInches++;
        if (scale != UnitsScaleIN && wholeInches == INCHES_PER_FOOT)
        {
            wholeFeet++;
            wholeInches = 0;
        }
    }
    else
    {
        fraction = [RCFraction fractionWithInches:remainingInchFraction];
        if ([fraction isEqualToOne])
        {
            fraction.nominator = 0;
            wholeInches++;
            if (scale != UnitsScaleIN && wholeInches == INCHES_PER_FOOT)
            {
                wholeFeet++;
                wholeInches = 0;
            }
        }
    }
}

- (void) roundToScale
{
    if (fraction && scale != UnitsScaleIN && scale != UnitsScaleFT)
    {
        if (fraction.floatValue >= .5) wholeInches++;
        fraction.nominator = 0;
    }
    if (scale != UnitsScaleIN && wholeInches == INCHES_PER_FOOT)
    {
        wholeFeet++;
        wholeInches = 0;
    }
    if (scale == UnitsScaleMI)
    {
        if (wholeInches >= 6) wholeFeet++;
    wholeInches = 0;
    }
    if (scale == UnitsScaleYD && wholeFeet == 3)
    {
        wholeYards++;
        wholeFeet = 0;
    }
    if (scale == UnitsScaleMI && wholeFeet == INCHES_PER_MILE / 12)
    {
        wholeMiles++;
    wholeFeet = 0;
    }
}

- (NSString*) description
{
    if (stringRep == nil)
    {
        NSMutableString* result = [[self getStringWithoutFractionOrUnitsSymbol] mutableCopy];

        if (scale != UnitsScaleYD && scale != UnitsScaleMI && fraction.nominator > 0)
        {
            if (result.length > 0) [result appendString:@" "];
            [result appendString:[fraction description]];
            [result appendString:@"\""];
        }
        else if (wholeInches > 0)
        {
            [result appendString:@"\""];
        }
        
        stringRep = [NSString stringWithString:result];
    }
    
    return stringRep;
}

- (NSString*) getStringWithoutFractionOrUnitsSymbol
{
    if (stringWithoutFractionOrUnitsSymbol == nil)
    {
        NSMutableString* result = [NSMutableString stringWithString:@""];
        
    [self roundToScale];
        
        if (scale == UnitsScaleMI)
        {
            [result appendString: [NSString localizedStringWithFormat:@"%imi", wholeMiles]];
        }
        if (scale == UnitsScaleYD)
        {
            [result appendString: [NSString localizedStringWithFormat:@"%iyd", wholeYards]];
        }
        if (scale == UnitsScaleFT && wholeFeet == 0)
        {
            [result appendString: @"0\'"];
        }
        else if (wholeFeet)
        {
            if (result.length > 0) [result appendString:@" "];
            [result appendString: [NSString localizedStringWithFormat:@"%i\'", wholeFeet]];
        }
        if (wholeInches && scale != UnitsScaleMI)
        {
            if (result.length > 0) [result appendString:@" "];
            [result appendString: [NSString localizedStringWithFormat:@"%i", wholeInches]];
        }
        if (scale == UnitsScaleIN && (wholeInches + fraction.nominator == 0))
        {
            [result appendString:@"0"];
        }
        
        stringWithoutFractionOrUnitsSymbol = [NSString stringWithString:result];;
    }
    
    return stringWithoutFractionOrUnitsSymbol;
}

@end
