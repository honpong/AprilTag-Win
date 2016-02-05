//
//  MPCircleLayerDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPCircleLayerDelegate.h"

@implementation MPCircleLayerDelegate

-(void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    CGFloat const lineWidth = 10.;
    CGFloat const xCenter = layer.frame.size.width / 2;
    CGFloat const yCenter = layer.frame.size.height / 2;
    CGFloat const circleRadius = xCenter - (lineWidth / 2);
    
    CGContextMoveToPoint(context, xCenter, yCenter);
    CGContextSetAlpha(context, 0.5);
    CGContextBeginPath(context);
    CGContextSetLineWidth(context, lineWidth);
    CGContextSetStrokeColorWithColor(context, [UIColor greenColor].CGColor);
    CGContextAddArc(context, xCenter, yCenter, circleRadius, (float)M_PI, -(float)M_PI, YES);
    CGContextClosePath(context);
    CGContextStrokePath(context);
}

// turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
