//
//  RCDistanceImperial.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceImperial.h"

@implementation RCDistanceImperial
@synthesize meters, scale;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)unitsScale
{
    if(self = [super init])
    {
        meters = distance;
        scale = unitsScale;
        convertedDist = meters * INCHES_PER_METER; //convert to inches
        
        if(scale == UnitsScaleFT)
        {
            convertedDist = convertedDist / INCHES_PER_FOOT;
            stringRep = [NSString localizedStringWithFormat:@"%0.2f\'", convertedDist];
        }
        else if(scale == UnitsScaleYD)
        {
            convertedDist = convertedDist / INCHES_PER_YARD;
            stringRep = [NSString localizedStringWithFormat:@"%0.2fyd", convertedDist];
        }
        else if(scale == UnitsScaleMI)
        {
            convertedDist = convertedDist / INCHES_PER_MILE;
            stringRep = [NSString localizedStringWithFormat:@"%0.3fmi", convertedDist];
        }
        else if(scale == UnitsScaleIN)
        {
            stringRep = [NSString localizedStringWithFormat:@"%0.1f\"", convertedDist];
        }
        else
        {
            //TODO: throw NSError
            stringRep = @"ERROR";
        }
    }
    return self;
}

- (NSString*) getString
{
    return stringRep;
}

+ (UnitsScale)autoSelectUnitsScale:(float)meters withUnits:(Units)units
{
    float inches = meters * INCHES_PER_METER;

    if (inches < INCHES_PER_FOOT) return UnitsScaleIN;
    if (inches >= INCHES_PER_MILE) return UnitsScaleMI;
    return UnitsScaleFT; //default
}

@end
