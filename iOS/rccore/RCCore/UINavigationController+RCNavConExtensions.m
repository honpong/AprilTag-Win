//
//  UINavigationController+RCNavConExtensions.m
//  RCCore
//
//  Created by Ben Hirashima on 10/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UINavigationController+RCNavConExtensions.h"

@implementation UINavigationController (RCNavConExtensions)

- (UIViewController *)secondToLastViewController
{
    UIViewController* secondToLast = nil;
    
    if (self.viewControllers.count >=2)
    {
        secondToLast = self.viewControllers[self.viewControllers.count - 2];
    }
    
    return secondToLast;
}

@end
