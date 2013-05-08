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
    CGContextBeginPath(context);
    CGContextAddArc(context, FEATURE_RADIUS + 2, FEATURE_RADIUS + 2, FEATURE_RADIUS, -M_PI, M_PI, 1);
    CGContextClosePath(context);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [layer.strokeColor CGColor]);
    CGContextStrokePath(context);
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
