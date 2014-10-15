//
//  RCModalCoverVerticalAnimation.m
//  RCCore
//
//  Created by Ben Hirashima on 10/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCModalCoverVerticalAnimation.h"
#import "UIView+RCConstraints.h"
#import "UIView+RCTappableView.h"

@implementation RCModalCoverVerticalAnimation

- (NSTimeInterval)transitionDuration:(id <UIViewControllerContextTransitioning>)transitionContext
{
    return 0.25f;
}

- (void)animateTransition:(id <UIViewControllerContextTransitioning>)transitionContext
{
    UIView *container = [transitionContext containerView];
    UIViewController *toVC = [transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];
    UIViewController *fromVC = [transitionContext viewControllerForKey:UITransitionContextFromViewControllerKey];
    UIView* shadeView = [UIView new];
    
    if (self.reverse)
    {
        [container insertSubview:toVC.view belowSubview:fromVC.view];
    }
    else
    {
        shadeView.backgroundColor = [UIColor blackColor];
        shadeView.alpha = 0;
        [container addSubview:shadeView];
        [shadeView addMatchSuperviewConstraints];
        [container addSubview:toVC.view];
        
        CGRect screenRect = [[UIScreen mainScreen] bounds];
        [toVC.view setFrame:CGRectMake(0, screenRect.size.height, fromVC.view.frame.size.width, fromVC.view.frame.size.height)];
    }
    
    [UIView animateWithDuration:[self transitionDuration:transitionContext]
                     animations:^{
                         if (self.reverse)
                         {
                             [fromVC.view setFrame:CGRectMake(0, fromVC.view.frame.size.height, fromVC.view.frame.size.width, fromVC.view.frame.size.height)];
                         }
                         else
                         {
                             shadeView.alpha = .5;
                             [toVC.view setFrame:CGRectMake(0, 0, fromVC.view.frame.size.width, fromVC.view.frame.size.height)];
                         }
                     }
                     completion:^(BOOL finished) {
                         [transitionContext completeTransition:YES];
                         // workaround for bug in iOS 8 http://stackoverflow.com/questions/24338700/from-view-controller-disappears-using-uiviewcontrollercontexttransitioning
                         [UIApplication.sharedApplication.keyWindow addSubview:toVC.view];
                     }];
}

@end
