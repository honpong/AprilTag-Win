//
//  MPDotLayerDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPDotLayerDelegate.h"
#import <GLKit/GLKit.h>

@implementation MPDotLayerDelegate

-(void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    CGFloat const xCenter = layer.frame.size.width / 2;
    CGFloat const yCenter = layer.frame.size.height / 2;
    CGFloat const circleRadius = 10.;
    
    CGContextMoveToPoint(context, xCenter, yCenter);
    CGContextSetAlpha(context, 0.5);
    CGContextSetFillColorWithColor(context, [UIColor colorWithRed:0 green:200 blue:255 alpha:1].CGColor);
    CGContextAddArc(context, xCenter, yCenter, circleRadius, (float)M_PI, -(float)M_PI, YES);
    CGContextFillPath(context);
}

// turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
