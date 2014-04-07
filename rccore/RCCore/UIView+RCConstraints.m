//
//  UIView+RCConstraints.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+RCConstraints.h"

@implementation UIView (RCConstraints)

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

- (NSLayoutConstraint*) addCenterXInSuperviewConstraints
{
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self
                                                                  attribute:NSLayoutAttributeCenterX
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self.superview
                                                                  attribute:NSLayoutAttributeCenterX
                                                                 multiplier:1.
                                                                   constant:0];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addCenterYInSuperviewConstraints
{
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self
                                                                  attribute:NSLayoutAttributeCenterY
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self.superview
                                                                  attribute:NSLayoutAttributeCenterY
                                                                 multiplier:1.
                                                                   constant:0];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (void) addWidthConstraint:(CGFloat)width andHeightConstraint:(CGFloat)height
{
    [self addConstraint: [self getWidthConstraint:width] ];
    
    [self addConstraint: [self getHeightConstraint:height] ];
}

- (NSLayoutConstraint*) getWidthConstraint:(CGFloat)width
{
    return [NSLayoutConstraint constraintWithItem:self
                                        attribute:NSLayoutAttributeWidth
                                        relatedBy:NSLayoutRelationEqual
                                           toItem:nil
                                        attribute:NSLayoutAttributeNotAnAttribute
                                       multiplier:1.
                                         constant:width];
}

- (NSLayoutConstraint*) getHeightConstraint:(CGFloat)height
{
    return [NSLayoutConstraint constraintWithItem:self
                                        attribute:NSLayoutAttributeHeight
                                        relatedBy:NSLayoutRelationEqual
                                           toItem:nil
                                        attribute:NSLayoutAttributeNotAnAttribute
                                       multiplier:1.
                                         constant:height];
}

- (NSLayoutConstraint*) addTopSpaceToSuperviewConstraint:(CGFloat)constant
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self.superview
                                                                  attribute:NSLayoutAttributeTop
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self
                                                                  attribute:NSLayoutAttributeTop
                                                                 multiplier:1.
                                                                   constant:constant];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addBottomSpaceToSuperviewConstraint:(CGFloat)constant
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self
                                                                  attribute:NSLayoutAttributeBottom
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self.superview
                                                                  attribute:NSLayoutAttributeBottom
                                                                 multiplier:1.
                                                                   constant:constant];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addLeadingSpaceToSuperviewConstraint:(CGFloat)constant
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self
                                                                  attribute:NSLayoutAttributeLeading
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self.superview
                                                                  attribute:NSLayoutAttributeLeft
                                                                 multiplier:1.
                                                                   constant:constant];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addTrailingSpaceToSuperviewConstraint:(CGFloat)constant
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self.superview
                                                                  attribute:NSLayoutAttributeRight
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self
                                                                  attribute:NSLayoutAttributeTrailing
                                                                 multiplier:1.
                                                                   constant:constant];
    [self.superview addConstraint:constraint];
    return constraint;
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

- (NSLayoutConstraint*) findTopToSuperviewConstraint
{
    for (NSLayoutConstraint* con in self.superview.constraints)
    {
        if (con.firstItem == self && con.secondItem == self.superview && con.firstAttribute == NSLayoutAttributeTop) return con;
    }
    return nil;
}

@end
