//
//  TMMeasurement.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/6/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"


@implementation TMMeasurement

@dynamic horzDist;
@dynamic name;
@dynamic pointToPoint;
@dynamic timestamp;
@dynamic totalPath;
@dynamic vertDist;

+ (NSString*)formattedDistance:(NSNumber*)dist withUnits:(int)units withFractional:(int)fractional
{
    NSString *unitsSymbol;
    NSNumber *convertedDist;
    
    if(units == UNITS_PREF_METRIC)
    {
        if(dist.floatValue >= 1.0)
        {
            unitsSymbol = @"m";
            return [NSString localizedStringWithFormat:@"%0.3f%@", dist.floatValue, unitsSymbol];
        }
        else
        {
            unitsSymbol = @"cm";
            convertedDist = [NSNumber numberWithFloat:dist.floatValue * 1000]; //convert to cm
            return [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist.floatValue, unitsSymbol];
        }
    }
    else
    {
        
        convertedDist = [NSNumber numberWithFloat:dist.floatValue * 39.3700787]; //convert to inches
        
        if(convertedDist.floatValue >= 12)
        {
            convertedDist = [NSNumber numberWithFloat:convertedDist.floatValue / 12]; //convert to feet
            unitsSymbol = @"\'";
            return [NSString localizedStringWithFormat:@"%0.3f%@", convertedDist.floatValue, unitsSymbol];
        }
        else
        {
            unitsSymbol = @"\"";
            return [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist.floatValue, unitsSymbol];
        }
    }
}

@end
