//
//  UIView+RCViewFade.m
//  RCCore
//
//  Created by Ben Hirashima on 5/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+RCViewFade.h"

@implementation UIView (RCViewFade)

-(void)fadeOutWithDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait
{
    [UIView beginAnimations: @"Fade Out" context:nil];
    
    // wait for time before begin
    [UIView setAnimationDelay:wait];
    
    // duration of animation
    [UIView setAnimationDuration:duration];
    self.alpha = 0.0;
    [UIView commitAnimations];
}

-(void)fadeInWithDuration:(NSTimeInterval)duration withAlpha:(float)alpha andWait:(NSTimeInterval)wait
{
    self.hidden = NO;
    self.alpha = 0;
    
    [UIView beginAnimations: @"Fade In" context:nil];
    
    // wait for time before begin
    [UIView setAnimationDelay:wait];
    
    // duration of animation
    [UIView setAnimationDuration:duration];
    self.alpha = alpha;
    [UIView commitAnimations];
}

-(void)fadeInWithDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait
{
    [self fadeInWithDuration:duration withAlpha:1.0 andWait:wait];
}

@end
