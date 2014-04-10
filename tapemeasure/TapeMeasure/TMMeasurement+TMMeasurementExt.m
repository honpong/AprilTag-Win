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

- (void) autoSelectUnitsScale
{
    float meters = [self getPrimaryMeasurementDist];

    if (meters < 1) self.unitsScaleMetric =  UnitsScaleCM;
    else if (meters >= 1000) self.unitsScaleMetric =  UnitsScaleKM;
    else self.unitsScaleMetric =  UnitsScaleM;

    float inches = meters * INCHES_PER_METER;

    if (inches < INCHES_PER_FOOT) self.unitsScaleImperial =  UnitsScaleIN;
    else if (inches >= INCHES_PER_MILE) self.unitsScaleImperial =  UnitsScaleMI;
    else self.unitsScaleImperial =  UnitsScaleFT; //default
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

- (id<RCDistance>) getPrimaryDistanceObject
{
    return [self getDistanceObject:[self getPrimaryMeasurementDist]];
}

- (id<RCDistance>) getDistanceObject:(float)meters
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

- (NSString*) getLocalizedDateString
{
    return [NSDateFormatter localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:self.timestamp]
                                          dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
}

@end
