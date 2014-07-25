//
//  MPFadeTransition.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPFadeTransition.h"

@implementation MPFadeTransition

static NSTimeInterval const MPAnimatedTransitionDuration = .3f;

- (void)animateTransition:(id<UIViewControllerContextTransitioning>)transitionContext
{
    UIViewController*fromViewController = [transitionContext viewControllerForKey:UITransitionContextFromViewControllerKey];
    UIViewController *toViewController = [transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];
    UIView *container = [transitionContext containerView];
    
    if (self.reverse) {
        fromViewController.view.alpha = 1.;
        [container insertSubview:toViewController.view belowSubview:fromViewController.view];
    }
    else {
        toViewController.view.alpha = 0;
        [container addSubview:toViewController.view];
    }
    
    [UIView animateKeyframesWithDuration:MPAnimatedTransitionDuration delay:0 options:0 animations:^{
        if (self.reverse) {
            fromViewController.view.alpha = 0;
        }
        else {
            toViewController.view.alpha = 1.;
        }
    } completion:^(BOOL finished) {
        [transitionContext completeTransition:finished];
    }];
}

- (NSTimeInterval)transitionDuration:(id<UIViewControllerContextTransitioning>)transitionContext
{
    return MPAnimatedTransitionDuration;
}

@end
