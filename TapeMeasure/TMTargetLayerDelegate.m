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
    CGContextSetFillColorWithColor(context, [UIColor blackColor].CGColor);
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius, -M_PI, M_PI, 1);
    CGContextFillPath(context);
    CGContextSetFillColorWithColor(context, [UIColor whiteColor].CGColor);
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius * 3 / 4, -M_PI, M_PI, 1);
    CGContextFillPath(context);
    CGContextSetFillColorWithColor(context, [UIColor blackColor].CGColor);
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius / 2, -M_PI, M_PI, 1);
    CGContextFillPath(context);
    CGContextSetFillColorWithColor(context, [UIColor whiteColor].CGColor);
    CGContextAddArc(context, circleRadius, circleRadius, circleRadius / 4, -M_PI, M_PI, 1);
    CGContextFillPath(context);
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end