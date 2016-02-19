//
//  RCOutlinedButton.m
//  RCCore
//
//  Created by Ben Hirashima on 8/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCOutlinedButton.h"

@implementation RCOutlinedButton

- (id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self initialize];
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [self initialize];
    }
    return self;
}

- (void) initialize
{
    
}

- (void) drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    CGFloat spacing = 1;
    CGFloat halfHeight = rect.size.height / 2.;
    CGFloat radius = halfHeight - spacing*2;
    CGPoint leftCenter = CGPointMake(halfHeight + spacing, halfHeight);
    CGPoint rightCenter = CGPointMake(rect.size.width - radius - spacing, halfHeight);
    CGFloat lineLength = rightCenter.x - leftCenter.x;
    
    CGContextBeginPath(context);
	CGContextAddArc(context, leftCenter.x, leftCenter.y, radius, M_PI_2, -M_PI_2, NO);
    CGContextAddLineToPoint(context, lineLength + radius + spacing, spacing*2);
    CGContextAddArc(context, rightCenter.x, rightCenter.y, radius, -M_PI_2, M_PI_2, NO);
    CGContextAddLineToPoint(context, radius + spacing, rect.size.height - spacing*2);
	CGContextClosePath(context);
    CGContextSetStrokeColorWithColor(context, [[UIColor whiteColor] CGColor]);
    CGContextSetLineWidth(context, 1.);
    CGContextStrokePath(context);
}

@end
