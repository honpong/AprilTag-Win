//
//  RCFeatureLayerDelegate.m
//  RCCore
//
//  Created by Ben Hirashima on 7/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFeatureLayerDelegate.h"

CGFloat const kMPFeatureRadius = 3;
CGFloat const kMPFeatureFrameSize = 10;

@implementation RCFeatureLayerDelegate
@synthesize color;

- (id) init
{
    if (self = [super init])
    {
        self.color = [UIColor colorWithRed:0 green:200 blue:255 alpha:1];
    }
    return self;
}

- (void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    CGFloat lineWidth = 2;
    CGFloat x = kMPFeatureRadius + lineWidth;
    CGFloat y = kMPFeatureRadius + lineWidth;
    CGFloat startAngle = -M_PI;
    CGFloat endAngle = M_PI;
    
    CGContextBeginPath(context);
    CGContextAddArc(context, x, y, kMPFeatureRadius, startAngle, endAngle, 1);
    CGContextClosePath(context);
    
    CGContextSetLineWidth(context, lineWidth);
    CGContextSetStrokeColorWithColor(context, [color CGColor]);
    CGContextStrokePath(context);
}

//turns off animations, reduces lag
- (id<CAAction>) actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
