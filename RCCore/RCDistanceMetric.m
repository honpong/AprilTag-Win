//
//  RCDistance.m
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCDistanceMetric.h"

@implementation RCDistanceMetric
@synthesize meters, scale, unitSymbol;

- (id) initWithMeters:(float)distance withScale:(UnitsScale)unitsScale
{
    if(self = [super init])
    {
        meters = fabsf(distance);
        scale = unitsScale;
        
        if(scale == UnitsScaleCM)
        {
            self.unitSymbol = @"cm";
            convertedDist = meters * 100; //convert to cm
            stringRep = [NSString localizedStringWithFormat:@"%0.1f%@", convertedDist, unitSymbol];
        }
        else if(scale == UnitsScaleKM)
        {
            self.unitSymbol = @"km";
            convertedDist = meters / 1000; //convert to km
            stringRep = [NSString localizedStringWithFormat:@"%0.3f%@", convertedDist, unitSymbol];
        }
        else if(scale == UnitsScaleM)
        {
            self.unitSymbol = @"m";
            stringRep = [NSString localizedStringWithFormat:@"%0.2f%@", meters, unitSymbol];
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

@end
