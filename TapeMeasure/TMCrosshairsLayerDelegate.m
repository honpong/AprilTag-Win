//
//  TMCrosshairsLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCrosshairsLayerDelegate.h"

@implementation TMCrosshairsLayerDelegate

- (id)initWithRadius:(float)radius
{
    self = [super init];
    if(self) {
        circleRadius = radius;
    }
    return self;
}

-(void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    float const xCenter = layer.frame.size.width / 2;
    float const yCenter = layer.frame.size.height / 2;
    
    CGContextAddArc(context, xCenter, yCenter, circleRadius, -M_PI, M_PI, 1);
    
    //line from top of screen to top of circle
    CGContextMoveToPoint(context, xCenter, 0);
    CGContextAddLineToPoint(context, xCenter, yCenter - circleRadius);
    
    //line from bottom of circle to bottom of screen
    CGContextMoveToPoint(context, xCenter, yCenter + circleRadius);
    CGContextAddLineToPoint(context, xCenter, layer.frame.size.height);
    
    //line from left of screen to left of circle
    CGContextMoveToPoint(context, 0, yCenter);
    CGContextAddLineToPoint(context, xCenter - circleRadius, yCenter);
    
    //line from right of circle to right of screen
    CGContextMoveToPoint(context, xCenter + circleRadius, yCenter);
    CGContextAddLineToPoint(context, layer.frame.size.width, yCenter);
    
    //horizontal crossbar
    CGContextMoveToPoint(context, xCenter - circleRadius / 4, yCenter);
    CGContextAddLineToPoint(context, xCenter + circleRadius / 4, yCenter);
    
    //vertical crossbar
    CGContextMoveToPoint(context, xCenter, yCenter - circleRadius / 4);
    CGContextAddLineToPoint(context, xCenter, yCenter + circleRadius / 4);
    
    
    CGContextSetAlpha(context, 1.0);
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
    CGContextStrokePath(context);
}

@end
