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
- (NSString*)getFormattedDistance:(float)meters
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

@end
