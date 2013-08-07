//
//  TMPoint+TMPointExt.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMPoint+TMPointExt.h"
#import "objc/runtime.h"

static char const * const FeatureKey = "feature";

@implementation TMPoint (TMPointExt)

- (float) distanceToPoint:(CGPoint)cgPoint
{
    return sqrtf(powf(cgPoint.x - self.imageX, 2) + powf(cgPoint.y - self.imageY, 2));
}

- (CGPoint) makeCGPoint
{
    return CGPointMake(self.imageX, self.imageY);
}

// see http://oleb.net/blog/2011/05/faking-ivars-in-objc-categories-with-associative-references/
- (RCFeaturePoint*)feature
{
    return objc_getAssociatedObject(self, FeatureKey);
}

- (void)setFeature:(RCFeaturePoint*)newFeature
{
    objc_setAssociatedObject(self, FeatureKey, newFeature, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end
