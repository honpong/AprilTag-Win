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
    // translates the context so that things are in the right place
    CGContextTranslateCTM(context, 0, -layer.frame.origin.y);
    
    CGContextBeginPath(context);
    CGContextAddArc(context, FEATURE_RADIUS + 2, FEATURE_RADIUS + 2, FEATURE_RADIUS, -M_PI, M_PI, 1);
    CGContextClosePath(context);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor colorWithRed:0 green:200 blue:255 alpha:1] CGColor]);
    CGContextStrokePath(context);
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
