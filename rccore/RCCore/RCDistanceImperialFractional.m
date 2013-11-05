//
//  RCDistanceImperialFractional.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceImperialFractional.h"

@implementation RCDistanceImperialFractional
{
    NSString* stringRep;
    float convertedDist;
}
@synthesize meters, scale, wholeMiles, wholeYards, wholeFeet, wholeInches, fraction;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)unitsScale
{
    if(self = [super init])
    {
        wholeMiles = wholeYards = wholeFeet = wholeInches = 0;
        meters = fabsf(distance);
        scale = unitsScale;
        
        convertedDist = meters * INCHES_PER_METER; //convert to inches
        
        if(scale == UnitsScaleFT)
        {
            [self calculateFeet:convertedDist];
        }
        else if(scale == UnitsScaleYD)
        {
            [self calculateYards:convertedDist];
        }
        else if(scale == UnitsScaleMI)
        {
            [self calculateMiles:convertedDist];
        }
        else if(scale == UnitsScaleIN)
        {
            [self calculateInches:convertedDist];
        }
        else
        {
            LOGME
            DLog(@"Unrecognized units scale");
        }
    }
    return self;
}

- (void) calculateMiles:(double)inches
{
    double miles = inches / INCHES_PER_MILE;
    wholeMiles = floor(miles);
    double remainingInches = inches - (wholeMiles * INCHES_PER_MILE);
    [self calculateFeet:remainingInches]; //skip to feet
}

- (void) calculateYards:(double)inches
{
    double yards = inches / INCHES_PER_YARD;
    wholeYards = floor(yards);
    double remainingInches = inches - (wholeYards * INCHES_PER_YARD);
    [self calculateFeet:remainingInches];
}

- (void) calculateFeet:(double)inches
{
    double feet = inches / INCHES_PER_FOOT;
    wholeFeet = floor(feet);
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

- (NSString*) getString
{
    NSMutableString* result = [self getStringWithoutFractionOrUnitsSymbol];

    if (scale != UnitsScaleYD && scale != UnitsScaleMI && fraction.nominator > 0)
    {
        if (result.length > 0) [result appendString:@" "];
        [result appendString:[fraction getString]];
        [result appendString:@"\""];
    }
    else if (wholeInches > 0)
    {
        [result appendString:@"\""];
    }

    return [NSString stringWithString:result];
}

- (NSMutableString*) getStringWithoutFractionOrUnitsSymbol
{
    NSMutableString* result = [NSMutableString stringWithString:@""];
    
    [self roundToScale];
    
    if (wholeMiles && scale == UnitsScaleMI) [result appendFormat:@"%imi", wholeMiles];
    if (wholeYards && scale == UnitsScaleYD)
    {
        if (result.length > 0) [result appendString:@" "];
        [result appendFormat:@"%iyd", wholeYards];
    }
    if (wholeFeet)
    {
        if (result.length > 0) [result appendString:@" "];
        [result appendFormat:@"%i\'", wholeFeet];
    }
    if (wholeInches && scale != UnitsScaleMI)
    {
        if (result.length > 0) [result appendString:@" "];
        [result appendFormat:@"%i", wholeInches];
    }
    if (wholeInches + wholeFeet + wholeYards + wholeMiles + fraction.nominator == 0)
    {
        [result appendString:@"0"];
    }
    
    return result;
}

@end
