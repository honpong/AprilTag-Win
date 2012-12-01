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
            return [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist.floatValue, unitsSymbol];
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

@end
