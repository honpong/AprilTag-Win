//
//  TMTableViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMTableViewController.h"

@interface TMTableViewController ()

@end

@implementation TMTableViewController

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
{
    return ((toInterfaceOrientation == UIInterfaceOrientationPortrait)
            || (toInterfaceOrientation == UIInterfaceOrientationPortraitUpsideDown)
            || (toInterfaceOrientation == UIInterfaceOrientationLandscapeLeft)
            || (toInterfaceOrientation == UIInterfaceOrientationLandscapeRight)
            );
}

- (NSUInteger)supportedInterfaceOrientations
{
    return (UIInterfaceOrientationMaskPortrait
            | UIInterfaceOrientationMaskPortraitUpsideDown
            | UIInterfaceOrientationLandscapeLeft
            | UIInterfaceOrientationLandscapeRight
            );
}

@end
