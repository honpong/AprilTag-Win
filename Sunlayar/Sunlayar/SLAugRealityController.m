//
//  SLAugRealityController.m
//  Sunlayar
//
//  Created by Ben Hirashima on 2/27/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import "SLAugRealityController.h"

@interface SLAugRealityController ()

@end

@implementation SLAugRealityController

- (instancetype)init
{
    self = [super init];
    if (!self) return nil;
    
    pageURL = [[NSBundle mainBundle] URLForResource:@"aug_reality" withExtension:@"html"];
    
    return self;
}

- (BOOL)prefersStatusBarHidden { return YES; }

- (NSUInteger)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscapeRight; }

@end
