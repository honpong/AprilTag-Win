//
//  TMPoint+TMPointExt.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMPoint.h"

@interface TMPoint (TMPointExt)

@property (nonatomic) RCFeaturePoint* feature;

- (float) distanceToPoint:(CGPoint)cgPoint;
- (CGPoint) makeCGPoint;

@end
