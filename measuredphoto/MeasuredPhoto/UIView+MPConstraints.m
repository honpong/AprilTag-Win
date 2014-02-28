//
//  UIView+MPConstraints.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+MPConstraints.h"

@implementation UIView (MPConstraints)

- (void) removeConstraintsFromSuperview
{
    for (NSLayoutConstraint *con in self.superview.constraints)
    {
        if (con.firstItem == self || con.secondItem == self)
        {
            [self.superview removeConstraint:con];
        }
    }
}

- (void) addMatchSuperviewConstraints
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
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
    self.translatesAutoresizingMaskIntoConstraints = NO;
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

- (void) modifyWidthContraint:(float)width andHeightConstraint:(float)height
{
    NSLayoutConstraint* widthConstraint = [self findWidthConstraint];
    if (widthConstraint != nil) widthConstraint.constant = width;
    
    NSLayoutConstraint* heightConstraint = [self findHeightConstraint];
    if (heightConstraint != nil) heightConstraint.constant = height;
}

- (NSLayoutConstraint*) findWidthConstraint
{
    for (NSLayoutConstraint* con in self.constraints)
    {
        if (con.firstItem == self && con.secondItem == nil && con.firstAttribute == NSLayoutAttributeWidth) return con;
    }
    return nil;
}

- (NSLayoutConstraint*) findHeightConstraint
{
    for (NSLayoutConstraint* con in self.constraints)
    {
        if (con.firstItem == self && con.secondItem == nil && con.firstAttribute == NSLayoutAttributeHeight) return con;
    }
    return nil;
}

@end
