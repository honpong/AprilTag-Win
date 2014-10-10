//
//  TMModalCoverVerticalTransitionDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/9/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMModalCoverVerticalTransitionDelegate.h"
#import "TMModalCoverVerticalTransition.h"

@implementation TMModalCoverVerticalTransitionDelegate

- (id <UIViewControllerAnimatedTransitioning>)animationControllerForPresentedController:(UIViewController *)presented presentingController:(UIViewController *)presenting sourceController:(UIViewController *)source
{
    TMModalCoverVerticalTransition *transitioning = [TMModalCoverVerticalTransition new];
    return transitioning;
}

- (id <UIViewControllerAnimatedTransitioning>)animationControllerForDismissedController:(UIViewController *)dismissed
{
    TMModalCoverVerticalTransition *transitioning = [TMModalCoverVerticalTransition new];
    transitioning.reverse = YES;
    return transitioning;
}

@end
