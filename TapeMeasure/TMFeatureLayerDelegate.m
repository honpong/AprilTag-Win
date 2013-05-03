//
//  TMFeaturesLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeatureLayerDelegate.h"

@implementation TMFeatureLayerDelegate

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    float const circleRadius = 2;
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius, -M_PI, M_PI, 1);
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
    CGContextStrokePath(context);
}

@end
