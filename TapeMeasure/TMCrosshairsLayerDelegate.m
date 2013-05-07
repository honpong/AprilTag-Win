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
    float const xCenter = -layer.frame.origin.x + layer.frame.size.width / 2;
    float const yCenter = -layer.frame.origin.y + layer.frame.size.height / 2; //TODO: figure out why this works
    int const crosshairLength = 50;
    
    CGContextAddArc(context, xCenter, yCenter, circleRadius, -M_PI, M_PI, 1);
    
    //line from top of screen to top of circle
    CGContextMoveToPoint(context, xCenter, yCenter - circleRadius - crosshairLength);
    CGContextAddLineToPoint(context, xCenter, yCenter - circleRadius);
    
    //line from bottom of circle to bottom of screen
    CGContextMoveToPoint(context, xCenter, yCenter + circleRadius);
    CGContextAddLineToPoint(context, xCenter, yCenter + circleRadius + crosshairLength);
    
    //line from left of screen to left of circle
    CGContextMoveToPoint(context, xCenter - circleRadius - crosshairLength, yCenter);
    CGContextAddLineToPoint(context, xCenter - circleRadius, yCenter);
    
    //line from right of circle to right of screen
    CGContextMoveToPoint(context, xCenter + circleRadius, yCenter);
    CGContextAddLineToPoint(context, xCenter + circleRadius + crosshairLength, yCenter);
    
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

 //turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
