//
//  TMCrosshairsLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCrosshairsLayerDelegate.h"

#define CROSSHAIR_LENGTH 30

@implementation TMCrosshairsLayerDelegate

-(void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    float const xCenter = layer.frame.size.width / 2;
    float const yCenter = layer.frame.size.height / 2;
        
    // horizontal crossbar
    CGContextMoveToPoint(context, xCenter - CROSSHAIR_LENGTH / 2, yCenter);
    CGContextAddLineToPoint(context, xCenter + CROSSHAIR_LENGTH / 2, yCenter);
    
    // vertical crossbar
    CGContextMoveToPoint(context, xCenter, yCenter - CROSSHAIR_LENGTH / 2);
    CGContextAddLineToPoint(context, xCenter, yCenter + CROSSHAIR_LENGTH / 2);
        
    CGContextSetAlpha(context, 1.0);
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
    CGContextStrokePath(context);
}

// turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
