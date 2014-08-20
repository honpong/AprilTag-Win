//
//  MPZoomTransition.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPZoomTransition.h"
#import "MPZoomTransitionDelegate.h"

static NSTimeInterval const MPAnimatedTransitionDuration = 0.5f;

@implementation MPZoomTransition

- (void)animateTransition:(id<UIViewControllerContextTransitioning>)transitionContext
{
    UIViewController<MPZoomTransitionFromView> *fromViewController = (UIViewController<MPZoomTransitionFromView>*)[transitionContext viewControllerForKey:UITransitionContextFromViewControllerKey];
    UIViewController *toViewController = [transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];
    UIView *container = [transitionContext containerView];
    
    UIView* fromView;
    if ([fromViewController respondsToSelector:@selector(transitionFromView)])
    {
        fromView = fromViewController.transitionFromView;
    }
    else
    {
        fromView = fromViewController.view;
    }
    
    if (self.reverse) {
        [container insertSubview:toViewController.view belowSubview:fromViewController.view];
    }
    else {
        CGAffineTransform scale = CGAffineTransformMakeScale(fromView.bounds.size.width / toViewController.view.bounds.size.width, fromView.bounds.size.height / toViewController.view.bounds.size.height);
        CGAffineTransform translation = CGAffineTransformMakeTranslation(fromView.center.x - toViewController.view.center.x, fromView.center.y - toViewController.view.center.y);
        toViewController.view.transform = CGAffineTransformConcat(scale, translation);
        [container addSubview:toViewController.view];
    }
    
    [UIView animateKeyframesWithDuration:MPAnimatedTransitionDuration delay:0 options:0 animations:^{
        if (self.reverse) {
            fromViewController.view.transform = CGAffineTransformMakeScale(0, 0);
        }
        else {
            toViewController.view.transform = CGAffineTransformIdentity;
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
