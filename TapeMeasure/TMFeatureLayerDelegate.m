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
    LOGME
    CGContextAddArc(context, FEATURE_RADIUS + 2, FEATURE_RADIUS + 2, FEATURE_RADIUS, -M_PI, M_PI, 1);
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
    CGContextStrokePath(context);
}

- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
