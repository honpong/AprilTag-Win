//
//  MPCrosshairsLayerDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPCrosshairsLayerDelegate.h"
#import <GLKit/GLKit.h>

static const int kCrosshairLength = 25;

@implementation MPCrosshairsLayerDelegate

-(void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    float const xCenter = layer.frame.size.width / 2;
    float const yCenter = layer.frame.size.height / 2;
    
    // horizontal crossbar
    CGContextMoveToPoint(context, xCenter - kCrosshairLength / 2, yCenter);
    CGContextAddLineToPoint(context, xCenter + kCrosshairLength / 2, yCenter);
    
    // vertical crossbar
    CGContextMoveToPoint(context, xCenter, yCenter - kCrosshairLength / 2);
    CGContextAddLineToPoint(context, xCenter, yCenter + kCrosshairLength / 2);
    
    CGContextSetAlpha(context, .8);
    CGContextSetLineWidth(context, 1.0);
    CGContextSetShouldAntialias(context, NO); // allows line to be thinner
    CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
    CGContextStrokePath(context);
}

// turns off animations, reduces lag
//- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
//{
//    return (id)[NSNull null];
//}

@end
