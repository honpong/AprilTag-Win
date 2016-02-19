//
//  MPExpandingCircleAnimationView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 12/3/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPExpandingCircleAnimationView.h"

@implementation MPExpandingCircleAnimationView
{
    CAShapeLayer* animationLayer;
    CABasicAnimation* scaleAnimation;
    CABasicAnimation* fadeAnimation;
    NSTimer* timer;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        CGMutablePathRef smallCircle;
        CGMutablePathRef largeCircle;
        
        smallCircle = CGPathCreateMutable();
        CGPathAddArc(smallCircle, NULL, self.bounds.size.width / 2, self.bounds.size.height / 2, 32, (float)M_PI, -(float)M_PI, NO);
        
        largeCircle = CGPathCreateMutable();
        CGPathAddArc(largeCircle, NULL, self.bounds.size.width / 2, self.bounds.size.height / 2, 60, (float)M_PI, -(float)M_PI, NO);
        
        animationLayer = [CAShapeLayer layer];
        animationLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
        animationLayer.lineWidth = 1;
        animationLayer.strokeColor = [[UIColor whiteColor] CGColor];
        animationLayer.fillColor = [[UIColor clearColor] CGColor];
        animationLayer.path = smallCircle;
        animationLayer.opacity = 0;
        
        [self.layer addSublayer:animationLayer];
        
        scaleAnimation = [CABasicAnimation animation];
        scaleAnimation.duration = 1.;
        scaleAnimation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
        scaleAnimation.fromValue = (__bridge_transfer id)(smallCircle);
        scaleAnimation.toValue = (__bridge_transfer id)largeCircle;
        
        fadeAnimation = [CABasicAnimation animation];
        fadeAnimation.duration = 1.;
        fadeAnimation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
        fadeAnimation.fromValue = [NSDecimalNumber numberWithFloat:.8];
        fadeAnimation.toValue = [NSDecimalNumber numberWithFloat:0];
    }
    return self;
}

- (void) startHighlightAnimation
{
    [self runHighlightAnimation]; // first run
    timer = [NSTimer scheduledTimerWithTimeInterval:1.3 target:self selector:@selector(runHighlightAnimation) userInfo:nil repeats:YES];
}

- (void) stopHighlightAnimation
{
    [timer invalidate];
    [animationLayer removeAllAnimations];
    animationLayer.opacity = 0;
}

- (void) runHighlightAnimation
{
    [animationLayer addAnimation:scaleAnimation forKey:@"path"];
    [animationLayer addAnimation:fadeAnimation forKey:@"opacity"];
}

@end
