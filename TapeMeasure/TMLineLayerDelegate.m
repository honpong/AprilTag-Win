//
//  TMLineLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 7/11/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLineLayerDelegate.h"

@implementation TMLineLayerDelegate
{
    CGPoint pointA;
    CGPoint pointB;
}

- (void) setPointA:(CGPoint)pointA_ andPointB:(CGPoint)pointB_
{
    pointA = pointA_;
    pointB = pointB_;
}

- (void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    CGContextClearRect(context, layer.frame); // erase previous drawing
    
    CGContextMoveToPoint(context, pointA.x, pointA.y);
    CGContextAddLineToPoint(context, pointB.x, pointB.y);
    
    CGContextSetAlpha(context, 1.0);
    CGContextSetLineWidth(context, 3);
    CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
    CGContextStrokePath(context);
}

@end
