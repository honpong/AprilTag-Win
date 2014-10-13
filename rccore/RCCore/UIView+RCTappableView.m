//
//  UIView+RCTappableView.m
//  RCCore
//
//  Created by Ben Hirashima on 10/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+RCTappableView.h"
#import <objc/runtime.h>

@implementation UIView (RCTappableView)

- (void (^)())onSingleTap
{
    return objc_getAssociatedObject(self, @selector(onSingleTap));
}

- (void)setOnSingleTap:(void (^)())onSingleTap
{
    objc_setAssociatedObject(self, @selector(onSingleTap), onSingleTap, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleSingleTap)];
    tapGesture.numberOfTapsRequired = 1;
    [self addGestureRecognizer:tapGesture];
}

- (void) handleSingleTap
{
    if (self.onSingleTap)
    {
        self.onSingleTap();
    }
}

@end
