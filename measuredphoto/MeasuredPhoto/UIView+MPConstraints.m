//
//  UIView+MPConstraints.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+MPConstraints.h"

@implementation UIView (MPConstraints)

- (void) addMatchSuperviewConstraints
{
    [self.superview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|[self]|"
                                                                 options:0
                                                                 metrics:nil
                                                                   views:NSDictionaryOfVariableBindings(self)]];
    
    [self.superview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|[self]|"
                                                                 options:0
                                                                 metrics:nil
                                                                   views:NSDictionaryOfVariableBindings(self)]];
}

- (void) addCenterInSuperviewConstraints
{
    [self addCenterXInSuperviewConstraints];
    [self addCenterYInSuperviewConstraints];
}

- (void) addCenterXInSuperviewConstraints
{
    [self.superview addConstraint:[NSLayoutConstraint constraintWithItem:self
                                                     attribute:NSLayoutAttributeCenterX
                                                     relatedBy:NSLayoutRelationEqual
                                                        toItem:self.superview
                                                     attribute:NSLayoutAttributeCenterX
                                                    multiplier:1.
                                                      constant:0]];
}

- (void) addCenterYInSuperviewConstraints
{
    [self.superview addConstraint:[NSLayoutConstraint constraintWithItem:self
                                                     attribute:NSLayoutAttributeCenterY
                                                     relatedBy:NSLayoutRelationEqual
                                                        toItem:self.superview
                                                     attribute:NSLayoutAttributeCenterY
                                                    multiplier:1.
                                                      constant:0]];
}

- (void) addWidthConstraint:(CGFloat)width andHeightConstraint:(CGFloat)height
{
    [self addConstraint:[NSLayoutConstraint constraintWithItem:self
                                                     attribute:NSLayoutAttributeWidth
                                                     relatedBy:NSLayoutRelationEqual
                                                        toItem:nil
                                                     attribute:NSLayoutAttributeNotAnAttribute
                                                    multiplier:1.
                                                      constant:width]];
    
    [self addConstraint:[NSLayoutConstraint constraintWithItem:self
                                                     attribute:NSLayoutAttributeHeight
                                                     relatedBy:NSLayoutRelationEqual
                                                        toItem:nil
                                                     attribute:NSLayoutAttributeNotAnAttribute
                                                    multiplier:1.
                                                      constant:height]];
}

@end
