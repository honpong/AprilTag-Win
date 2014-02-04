//
//  MPDotLayerDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPDotLayerDelegate.h"

@implementation MPDotLayerDelegate

-(void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    float const xCenter = layer.frame.size.width / 2;
    float const yCenter = layer.frame.size.height / 2;
    float const circleRadius = 10.;
    
    CGContextMoveToPoint(context, xCenter, yCenter);
    CGContextSetAlpha(context, 0.5);
    CGContextSetFillColorWithColor(context, [UIColor colorWithRed:0 green:200 blue:255 alpha:1].CGColor);
    CGContextAddArc(context, xCenter, yCenter, circleRadius, -M_PI, M_PI, 1);
    CGContextFillPath(context);
}

@end
