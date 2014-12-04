//
//  MPShutterButtonView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPShutterButton.h"
#import <RCCore/RCCore.h>
#import "MPCapturePhoto.h"

@implementation MPShutterButton
{
    CAShapeLayer* highlightLayer;
    CABasicAnimation* scaleAnimation;
    CABasicAnimation* fadeAnimation;
    CGMutablePathRef smallCircle;
    CGMutablePathRef largeCircle;
    NSTimer* timer;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
        {
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(handleOrientationChange:)
                                                         name:MPUIOrientationDidChangeNotification
                                                       object:nil];
        }
        
        smallCircle = CGPathCreateMutable();
        CGPathAddArc(smallCircle, NULL, self.bounds.size.width / 2, self.bounds.size.height / 2, 28, (CGFloat)M_PI, -(CGFloat)M_PI, NO);
        CGPathCloseSubpath(smallCircle);
        
        largeCircle = CGPathCreateMutable();
        CGPathAddArc(largeCircle, NULL, self.bounds.size.width / 2, self.bounds.size.height / 2, 60, (CGFloat)M_PI, -(CGFloat)M_PI, NO);
        CGPathCloseSubpath(largeCircle);
        
        highlightLayer = [CAShapeLayer layer];
        highlightLayer.frame = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
        highlightLayer.lineWidth = 1;
        highlightLayer.strokeColor = [[UIColor whiteColor] CGColor];
        highlightLayer.fillColor = [[UIColor clearColor] CGColor];
        highlightLayer.path = smallCircle;
        highlightLayer.opacity = 0;
        
        [self.layer addSublayer:highlightLayer];
        
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

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    CFRelease(smallCircle);
    CFRelease(largeCircle);
}

- (void) handleOrientationChange:(NSNotification*)notification
{
    if (notification.object)
    {
        MPOrientationChangeData* data = (MPOrientationChangeData*)notification.object;
        [self applyRotationTransformation:data.orientation animated:data.animated];
    }
}

- (void) startHighlightAnimation
{
    [self runHighlightAnimation]; // first run
    timer = [NSTimer scheduledTimerWithTimeInterval:1.3 target:self selector:@selector(runHighlightAnimation) userInfo:nil repeats:YES];
}

- (void) stopHighlightAnimation
{
    [timer invalidate];
    [highlightLayer removeAllAnimations];
    highlightLayer.opacity = 0;
}

- (void) runHighlightAnimation
{
    [highlightLayer addAnimation:scaleAnimation forKey:@"path"];
    [highlightLayer addAnimation:fadeAnimation forKey:@"opacity"];
}

@end