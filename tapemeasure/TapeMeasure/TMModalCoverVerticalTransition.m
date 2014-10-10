//
//  TMModalCoverVerticalTransition.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMModalCoverVerticalTransition.h"
#import <RCCore/RCCore.h>

@implementation TMModalCoverVerticalTransition

static NSTimeInterval const MPAnimatedTransitionDuration = .3f;

- (void)animateTransition:(id<UIViewControllerContextTransitioning>)transitionContext
{
    UIViewController*fromViewController = [transitionContext viewControllerForKey:UITransitionContextFromViewControllerKey];
    UIViewController *toViewController = [transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];
    UIView *container = [transitionContext containerView];
    NSLayoutConstraint* bottomConstraint;
    
    if (self.reverse) {
        [container insertSubview:toViewController.view belowSubview:fromViewController.view];
        bottomConstraint = [fromViewController.view findBottomToSuperviewConstraint];
    }
    else {
        [container addSubview:toViewController.view];
        [toViewController.view removeConstraintsFromSuperview];
        [toViewController.view addWidthConstraint:320 andHeightConstraint:265];
        bottomConstraint = [toViewController.view addBottomSpaceToSuperviewConstraint:-320];
        [toViewController.view addCenterXInSuperviewConstraints];
    }
    
    [container layoutIfNeeded];
    
    [UIView animateKeyframesWithDuration:MPAnimatedTransitionDuration delay:0 options:0 animations:^{
        if (self.reverse) {
            bottomConstraint.constant = -320;
        }
        else {
            bottomConstraint.constant = 0;
        }
        
        [container setNeedsUpdateConstraints];
        [container layoutIfNeeded];
    } completion:^(BOOL finished) {
        [transitionContext completeTransition:finished];
    }];
}

- (NSTimeInterval)transitionDuration:(id<UIViewControllerContextTransitioning>)transitionContext
{
    return MPAnimatedTransitionDuration;
}

@end
