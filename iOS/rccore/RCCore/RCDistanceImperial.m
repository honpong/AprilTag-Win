//
//  RCDistanceImperial.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceImperial.h"
#import "RCDebugLog.h"

@implementation RCDistanceImperial
{
    NSString* stringRep;
    float convertedDist;
}
@synthesize meters, scale;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)unitsScale
{
    if(self = [super init])
    {
        meters = fabsf(distance);
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
            stringRep = @"";
            DLog(@"Unknown unit scale");
        }
    }
    return self;
}

- (NSString*) description
{
    return stringRep;
}

@end
