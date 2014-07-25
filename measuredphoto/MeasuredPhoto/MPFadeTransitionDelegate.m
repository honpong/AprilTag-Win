//
//  MPFadeTransitionDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPFadeTransitionDelegate.h"
#import "MPFadeTransition.h"

@implementation MPFadeTransitionDelegate

- (id <UIViewControllerAnimatedTransitioning>)animationControllerForPresentedController:(UIViewController *)presented presentingController:(UIViewController *)presenting sourceController:(UIViewController *)source
{
    MPFadeTransition *transitioning = [MPFadeTransition new];
    return transitioning;
}

- (id <UIViewControllerAnimatedTransitioning>)animationControllerForDismissedController:(UIViewController *)dismissed
{
    MPFadeTransition *transitioning = [MPFadeTransition new];
    transitioning.reverse = YES;
    return transitioning;
}

@end
