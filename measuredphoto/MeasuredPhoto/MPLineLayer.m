//
//  TMLineLayer.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 7/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPLineLayer.h"

@implementation MPLineLayer
@synthesize pointA, pointB;

- (id) initWithPointA:(CGPoint)pointA_ andPointB:(CGPoint)pointB_
{
    if (self = [super init])
    {
        pointA = pointA_;
        pointB = pointB_;
    }
    return self;
}

@end
