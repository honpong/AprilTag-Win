//
//  MPView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 1/22/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPView.h"

@implementation MPView

- (void) constrainToSelf:(UIView*)view
{
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|[view]|"
                                                                 options:0
                                                                 metrics:nil
                                                                   views:NSDictionaryOfVariableBindings(view)]];
    [self addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|[view]|"
                                                                 options:0
                                                                 metrics:nil
                                                                   views:NSDictionaryOfVariableBindings(view)]];
}

@end
