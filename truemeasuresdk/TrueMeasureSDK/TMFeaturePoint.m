//
//  TMFeaturePoint.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/21/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeaturePoint.h"

@implementation TMFeaturePoint

- (id) initWithX:(float)x withY:(float)y withOriginalDepth:(TMScalar *)originalDepth withWorldPoint:(TMPoint *)worldPoint withInitialized:(bool)initialized
{
    if(self = [super init])
    {
        _x = x;
        _y = y;
        _originalDepth = originalDepth;
        _worldPoint = worldPoint;
        _initialized = initialized;
    }
    return self;
}

- (float) pixelDistanceToPoint:(CGPoint)cgPoint
{
    return sqrtf(powf(cgPoint.x - self.x, 2) + powf(cgPoint.y - self.y, 2));
}

- (CGPoint) makeCGPoint
{
    return CGPointMake(self.x, self.y);
}

@end
