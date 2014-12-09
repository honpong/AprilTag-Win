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
    [UIView animateWithDuration: duration
                          delay: wait
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.alpha = 0;
                     }
                     completion:nil];
}

-(void)fadeInWithDuration:(NSTimeInterval)duration withAlpha:(float)alpha andWait:(NSTimeInterval)wait
{
    self.hidden = NO;
    self.alpha = 0;
    
    [UIView animateWithDuration: duration
                          delay: wait
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.alpha = alpha;
                     }
                     completion:nil];
}

-(void)fadeInWithDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait
{
    [self fadeInWithDuration:duration withAlpha:1.0 andWait:wait];
}

@end
