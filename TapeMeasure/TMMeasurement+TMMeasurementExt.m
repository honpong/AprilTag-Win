//
//  TMMeasurement+TMMeasurementExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement+TMMeasurementExt.h"

@implementation TMMeasurement (TMMeasurementExt)

#pragma mark - Database operations

+ (TMMeasurement*) getNewMeasurement
{
    TMMeasurement* new = (TMMeasurement*)[DATA_MANAGER getNewObjectOfType:[self getEntity]];
    new.units = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_UNITS];
    return new;
}

+ (TMMeasurement*)getMeasurementById:(int)dbid
{
    return (TMMeasurement*)[DATA_MANAGER getObjectOfType:[self getEntity] byDbid:dbid];
}

#pragma mark - Misc

/** Gets formatted distance string according to this instance's properties, such as units and units scale */
- (NSString*) getFormattedDistance:(float)meters
{
    if((Units)self.units == UnitsImperial)
    {
        return [RCDistanceFormatter getFormattedDistance:meters
                                               withUnits:self.units
                                               withScale:self.unitsScaleImperial
                                          withFractional:self.fractional];
    }
    else
    {
        return [RCDistanceFormatter getFormattedDistance:meters
                                               withUnits:self.units
                                               withScale:self.unitsScaleMetric
                                          withFractional:self.fractional];
    }
}

- (void) autoSelectUnitsScale
{
    float primaryMeasurement = [self getPrimaryMeasurementDist];
    self.unitsScaleImperial = [RCDistanceFormatter autoSelectUnitsScale:primaryMeasurement withUnits:UnitsImperial];
    self.unitsScaleMetric = [RCDistanceFormatter autoSelectUnitsScale:primaryMeasurement withUnits:UnitsMetric];
}

- (float) getPrimaryMeasurementDist
{
    switch (self.type) {
        case TypeTotalPath:
            return self.totalPath;
            
        case TypeHorizontal:
            return self.horzDist;
                        
        case TypeVertical:
            return self.zDisp;
            
        default:
            return self.pointToPoint;
    }
}

- (id<RCDistance>) getPrimaryDistance
{
    return [self getDistance:[self getPrimaryMeasurementDist]];
}

- (id<RCDistance>) getDistance:(float)meters
{
    if (self.units == UnitsImperial)
    {
        if (self.fractional)
        {
            return [[RCDistanceImperialFractional alloc] initWithMeters:meters withScale:self.unitsScaleImperial];
        }
        else
        {
            return [[RCDistanceImperial alloc] initWithMeters:meters withScale:self.unitsScaleImperial];
        }
    }
    else
    {
        return [[RCDistanceMetric alloc] initWithMeters:meters withScale:self.unitsScaleMetric];
    }
}

@end
