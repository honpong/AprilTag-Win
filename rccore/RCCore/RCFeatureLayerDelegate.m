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
    CGFloat const lineWidth = 2;
    CGFloat const x = kMPFeatureRadius + lineWidth;
    CGFloat const y = kMPFeatureRadius + lineWidth;
    
    CGContextBeginPath(context);
    CGContextAddArc(context, x, y, kMPFeatureRadius, M_PI, -M_PI, YES);
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
