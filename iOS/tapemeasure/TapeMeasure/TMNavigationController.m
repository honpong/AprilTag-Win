//
//  TMNavigationController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 4/11/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMNavigationController.h"

@interface TMNavigationController ()

@end

@implementation TMNavigationController

- (BOOL) shouldAutorotate
{
    return YES;
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
}

@end
