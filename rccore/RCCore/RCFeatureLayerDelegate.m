//
//  RCFeatureLayerDelegate.m
//  RCCore
//
//  Created by Ben Hirashima on 7/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFeatureLayerDelegate.h"

CGFloat const kMPFeatureRadius = 2;
CGFloat const kMPFeatureFrameSize = 4;

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
    CGContextAddArc(context, kMPFeatureRadius, kMPFeatureRadius, kMPFeatureRadius, (float)M_PI, -(float)M_PI, NO);
    CGContextClosePath(context);
    
    CGContextSetShouldAntialias(context, YES);
    CGContextSetFillColorWithColor(context, [color CGColor]);
    CGContextFillPath(context);
}

//turns off animations, reduces lag
- (id<CAAction>) actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
