//
//  RCModalCoverVerticalTransition.m
//  RCCore
//
//  Created by Ben Hirashima on 10/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCModalCoverVerticalTransition.h"
#import "RCModalCoverVerticalAnimation.h"
#import <UIKit/UIKit.h>

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

@end
