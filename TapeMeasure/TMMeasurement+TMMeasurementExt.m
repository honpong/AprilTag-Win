//
//  TMMeasurement+TMMeasurementExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasurement+TMMeasurementExt.h"

@implementation TMMeasurement (TMMeasurementExt)

/** Gets formatted distance string according to this instance's properties, such as units and units scale */
- (NSString*)getFormattedDistance:(NSNumber *)meters
{
    if((Units)self.units.intValue == UnitsImperial)
    {
        return [RCDistanceFormatter getFormattedDistance:meters.floatValue
                                               withUnits:self.units.intValue
                                               withScale:self.unitsScaleImperial.intValue
                                          withFractional:self.fractional.boolValue];
    }
    else
    {
        return [RCDistanceFormatter getFormattedDistance:meters.floatValue
                                               withUnits:self.units.intValue
                                               withScale:self.unitsScaleMetric.intValue
                                          withFractional:self.fractional.boolValue];
    }
}

@end
