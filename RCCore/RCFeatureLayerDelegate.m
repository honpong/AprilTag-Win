//
//  RCFeatureLayerDelegate.m
//  RCCore
//
//  Created by Ben Hirashima on 7/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFeatureLayerDelegate.h"

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
    CGContextBeginPath(context);
    CGContextAddArc(context, FEATURE_RADIUS + 2, FEATURE_RADIUS + 2, FEATURE_RADIUS, -M_PI, M_PI, 1);
    CGContextClosePath(context);
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [color CGColor]);
    CGContextStrokePath(context);
}

//turns off animations, reduces lag
- (id<CAAction>) actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
