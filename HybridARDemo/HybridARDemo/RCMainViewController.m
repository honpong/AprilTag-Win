//
//  RCMainViewController.m
//  HybridARDemo
//
//  Created by Ben Hirashima on 5/19/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import "RCMainViewController.h"

@implementation RCMainViewController

- (instancetype)init
{
    self = [super init];
    if (!self) return nil;
    
    pageURL = [[NSBundle mainBundle] URLForResource:@"main" withExtension:@"html"];
    
    return self;
}

- (BOOL)prefersStatusBarHidden { return YES; }

- (NSUInteger)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscapeRight; }

@end
