//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCTranslation.h"

@implementation RCTranslation

- (RCPoint *)transformPoint:(RCPoint *)point
{
    vFloat stdSum = vsqrtf(point.standardDeviation * point.standardDeviation + self.standardDeviation * self.standardDeviation);
    return [[RCPoint alloc] initWithVector:point.vector + self.vector withStandardDeviation:stdSum];
}

- (RCScalar *)getDistance
{
    vFloat square = self.vector * self.vector;
    float dist = sqrtf(square[0] + square[1] + square[2]);
    vFloat lin = self.standardDeviation * self.vector / (vFloat) { dist, dist, dist, dist };
    square = lin * lin;
    float std = sqrtf(square[0] + square[1] + square[2]);
    return [[RCScalar alloc] initWithScalar:dist withStdDev:std];
}

- (RCTranslation *)getInverse
{
    return [[RCTranslation alloc] initWithVector:-self.vector withStandardDeviation:self.standardDeviation];
}

- (RCTranslation *)composeWithTranslation:(RCTranslation *)other
{
    vFloat stdSum = vsqrtf(other.standardDeviation * other.standardDeviation + self.standardDeviation * self.standardDeviation);
    return [[RCTranslation alloc] initWithVector:other.vector + self.vector withStandardDeviation:stdSum];
}

+ (RCTranslation *)translationFromPoint:(RCPoint *)fromPoint toPoint:(RCPoint *)toPoint
{
    vFloat trans = toPoint.vector - fromPoint.vector;
    vFloat stdev = vsqrtf(toPoint.standardDeviation * toPoint.standardDeviation + fromPoint.standardDeviation * fromPoint.standardDeviation);
    return [[RCTranslation alloc] initWithVector:trans withStandardDeviation:stdev];
}

@end