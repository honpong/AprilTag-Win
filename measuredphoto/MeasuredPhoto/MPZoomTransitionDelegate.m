//
//  MPZoomTransitionDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/24/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPZoomTransitionDelegate.h"
#import "MPZoomTransition.h"

@implementation MPZoomTransitionDelegate

- (id <UIViewControllerAnimatedTransitioning>)animationControllerForPresentedController:(UIViewController *)presented presentingController:(UIViewController *)presenting sourceController:(UIViewController *)source
{
    MPZoomTransition *transitioning = [MPZoomTransition new];
    return transitioning;
}

- (id <UIViewControllerAnimatedTransitioning>)animationControllerForDismissedController:(UIViewController *)dismissed
{
    MPZoomTransition *transitioning = [MPZoomTransition new];
    transitioning.reverse = YES;
    return transitioning;
}

@end
