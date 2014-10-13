//
//  RCModalCoverVerticalTransition.m
//  RCCore
//
//  Created by Ben Hirashima on 10/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCModalCoverVerticalTransition.h"
#import "RCModalCoverVerticalAnimation.h"

@implementation RCModalCoverVerticalTransition

- (id <UIViewControllerAnimatedTransitioning>)animationControllerForPresentedController:(UIViewController *)presented presentingController:(UIViewController *)presenting sourceController:(UIViewController *)source
{
    RCModalCoverVerticalAnimation *controller = [RCModalCoverVerticalAnimation new];
    controller.reverse = NO;
    return controller;
}

- (id <UIViewControllerAnimatedTransitioning>)animationControllerForDismissedController:(UIViewController *)dismissed
{
    RCModalCoverVerticalAnimation *controller = [RCModalCoverVerticalAnimation new];
    controller.reverse = YES;
    return controller;
}

- (id <UIViewControllerInteractiveTransitioning>)interactionControllerForPresentation:(id <UIViewControllerAnimatedTransitioning>)animator
{
    return nil;
}

- (id <UIViewControllerInteractiveTransitioning>)interactionControllerForDismissal:(id <UIViewControllerAnimatedTransitioning>)animator
{
    return nil;
}

@end
