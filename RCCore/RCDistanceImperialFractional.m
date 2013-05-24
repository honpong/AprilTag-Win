//
//  RCDistanceImperialFractional.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceImperialFractional.h"

@implementation RCDistanceImperialFractional
@synthesize meters, scale, wholeMiles, wholeYards, wholeFeet, wholeInches, fraction;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)unitsScale
{
    if(self = [super init])
    {
        wholeMiles = wholeYards = wholeFeet = wholeInches = 0;
        meters = distance;
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
            NSLog(@"Unrecognized units scale");
        }
    }
    return self;
}

- (void) calculateMiles:(double)inches
{
    double miles = inches / INCHES_PER_MILE;
    wholeMiles = floor(miles);
    double remainingInches = inches - (wholeMiles * INCHES_PER_MILE);
    
    [self calculateYards:remainingInches];
}

- (void) calculateYards:(double)inches
{
    double yards = inches / INCHES_PER_YARD;
    wholeYards = floor(yards);
    double remainingInches = inches - (wholeYards * INCHES_PER_YARD);
    if (remainingInches > (INCHES_PER_YARD - 1))
        wholeYards++;
    else
        [self calculateFeet:remainingInches];
}

- (void) calculateFeet:(double)inches
{
    double feet = inches / INCHES_PER_FOOT;
    wholeFeet = floor(feet);
    double remainingInches = inches - (wholeFeet * INCHES_PER_FOOT);
    if (remainingInches >= INCHES_PER_FOOT - THIRTYSECOND_INCH)
        wholeFeet++;
    else
        [self calculateInches:remainingInches];
}

- (void) calculateInches:(double)inches
{
    wholeInches = floor(inches);
    double remainingInchFraction = inches - wholeInches;
    if (remainingInchFraction >= 1 - THIRTYSECOND_INCH)
        wholeInches++;
    else
        fraction = [RCFraction fractionWithInches:remainingInchFraction];
}

- (NSString*) getString
{
    NSMutableString* result = [NSMutableString stringWithString:@""];
    
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
    if (scale != UnitsScaleYD && scale != UnitsScaleMI && fraction.nominator > 0)
    {
        if (result.length > 0) [result appendString:@" "];
        [result appendString:[fraction getString]];
    }
    if ((scale == UnitsScaleFT || scale == UnitsScaleIN) && (wholeInches > 0 || fraction.nominator > 0)) [result appendString:@"\""];
    
    return [NSString stringWithString:result];
}

//TODO: DRY it off
+ (UnitsScale)autoSelectUnitsScale:(float)meters withUnits:(Units)units
{
    float inches = meters * INCHES_PER_METER;
    
    if (inches < INCHES_PER_FOOT) return UnitsScaleIN;
    if (inches >= INCHES_PER_MILE) return UnitsScaleMI;
    return UnitsScaleFT; //default
}

@end
