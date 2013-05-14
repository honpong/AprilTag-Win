//
//  TMTickMarksLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMTickMarksLayerDelegate.h"

@implementation TMTickMarksLayerDelegate

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    int centerX = layer.frame.size.width / 2;
    int zeroY = layer.frame.origin.y;
    
//    CGContextBeginPath(context);
    CGContextMoveToPoint(context, centerX, zeroY);
    CGContextAddLineToPoint(context, centerX, 10);
//    CGContextClosePath(context);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor blackColor] CGColor]);
    CGContextStrokePath(context);
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
