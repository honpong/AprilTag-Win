//
//  TMTargetLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMTargetLayerDelegate.h"

@implementation TMTargetLayerDelegate

- (id)initWithRadius:(float)radius
{
    self = [super init];
    if(self) {
        circleRadius = radius;
    }
    return self;
}

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    CGContextSetAlpha(context, 0.3);
    
    //draw concentric circles for the target
    CGContextSetFillColorWithColor(context, [UIColor colorWithRed:0.7 green:0 blue:0 alpha:0.3].CGColor);
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius, -M_PI, M_PI, 1);
    CGContextFillPath(context);
    CGContextSetFillColorWithColor(context, [UIColor colorWithRed:1 green:1 blue:1 alpha:0.4].CGColor);
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius * 3 / 4, -M_PI, M_PI, 1);
    CGContextFillPath(context);
    CGContextSetFillColorWithColor(context, [UIColor colorWithRed:0.7 green:0 blue:0 alpha:0.3].CGColor);
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius / 2, -M_PI, M_PI, 1);
    CGContextFillPath(context);
    CGContextSetFillColorWithColor(context, [UIColor colorWithRed:1 green:1 blue:1 alpha:0.4].CGColor);
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius / 4, -M_PI, M_PI, 1);
    CGContextFillPath(context);
}
@end