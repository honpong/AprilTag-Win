//
//  TMLineLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 7/11/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLineLayerDelegate.h"
#import "TMLineLayer.h"

@implementation TMLineLayerDelegate

- (void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    TMLineLayer* lineLayer = (TMLineLayer*)layer;
    
    CGContextMoveToPoint(context, lineLayer.pointA.x, lineLayer.pointA.y);
    CGContextAddLineToPoint(context, lineLayer.pointB.x, lineLayer.pointB.y);
    
    CGContextSetAlpha(context, 1.0);
    CGContextSetLineWidth(context, 3);
    CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
    CGContextStrokePath(context);
}

@end
