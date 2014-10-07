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
    [self addMatchWidthToSuperviewConstraints];
    [self addMatchHeightToSuperviewContraints];
}

- (void) addMatchSuperviewConstraints:(CGFloat)spacing
{
    [self addMatchWidthToSuperviewConstraints:spacing];
    [self addMatchHeightToSuperviewContraints:spacing];
}

- (void) addMatchWidthToSuperviewConstraints
{
    [self addMatchWidthToSuperviewConstraints:0];
}

- (void) addMatchWidthToSuperviewConstraints:(CGFloat)spacing
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    [self.superview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:|-spacing-[self]-spacing-|"
                                                                           options:0
                                                                           metrics:@{ @"spacing": [NSNumber numberWithDouble:spacing] }
                                                                             views:NSDictionaryOfVariableBindings(self)]];
}

- (void) addMatchHeightToSuperviewContraints
{
    [self addMatchHeightToSuperviewContraints:0];
}

- (void) addMatchHeightToSuperviewContraints:(CGFloat)spacing
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    [self.superview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:|-spacing-[self]-spacing-|"
                                                                           options:0
                                                                           metrics:@{ @"spacing": [NSNumber numberWithDouble:spacing] }
                                                                             views:NSDictionaryOfVariableBindings(self)]];
}

- (void) addCenterInSuperviewConstraints
{
    [self addCenterXInSuperviewConstraints];
    [self addCenterYInSuperviewConstraints];
}

- (NSLayoutConstraint*) addCenterXInSuperviewConstraints
{
    return [self addCenterXInSuperviewConstraints:0];
}

- (NSLayoutConstraint*) addCenterXInSuperviewConstraints:(CGFloat)offset
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self
                                                                  attribute:NSLayoutAttributeCenterX
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self.superview
                                                                  attribute:NSLayoutAttributeCenterX
                                                                 multiplier:1.
                                                                   constant:offset];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addCenterYInSuperviewConstraints
{
    return [self addCenterYInSuperviewConstraints:0];
}

- (NSLayoutConstraint*) addCenterYInSuperviewConstraints:(CGFloat)offset
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self
                                                                  attribute:NSLayoutAttributeCenterY
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self.superview
                                                                  attribute:NSLayoutAttributeCenterY
                                                                 multiplier:1.
                                                                   constant:offset];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (void) addWidthConstraint:(CGFloat)width andHeightConstraint:(CGFloat)height
{
    [self addWidthConstraint:width];
    [self addHeightConstraint:height];
}

- (NSLayoutConstraint*) addWidthConstraint:(CGFloat)width
{
    NSLayoutConstraint* widthConstraint = [self getWidthConstraint:width];
    [self addConstraint:widthConstraint];
    return widthConstraint;
}

- (NSLayoutConstraint*) addHeightConstraint:(CGFloat)height
{
    NSLayoutConstraint* heightConstraint = [self getHeightConstraint:height];
    [self addConstraint:heightConstraint];
    return heightConstraint;
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
                                                                   constant:-constant];
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
                                                                   constant:-constant];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addLeftSpaceToSuperviewConstraint:(CGFloat)constant
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self
                                                                  attribute:NSLayoutAttributeLeft
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self.superview
                                                                  attribute:NSLayoutAttributeLeft
                                                                 multiplier:1.
                                                                   constant:constant];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addRightSpaceToSuperviewConstraint:(CGFloat)constant
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self.superview
                                                                  attribute:NSLayoutAttributeRight
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self
                                                                  attribute:NSLayoutAttributeRight
                                                                 multiplier:1.
                                                                   constant:constant];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addTopSpaceToViewConstraint:(UIView*)view withDist:(int)dist
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:view
                                                                  attribute:NSLayoutAttributeBottom
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self
                                                                  attribute:NSLayoutAttributeTop
                                                                 multiplier:1.
                                                                   constant:-dist];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addBottomSpaceToViewConstraint:(UIView*)view withDist:(int)dist
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:view
                                                                  attribute:NSLayoutAttributeTop
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self
                                                                  attribute:NSLayoutAttributeBottom
                                                                 multiplier:1.
                                                                   constant:dist];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addLeftSpaceToViewConstraint:(UIView*)view withDist:(int)dist
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:self
                                                                  attribute:NSLayoutAttributeLeft
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:view
                                                                  attribute:NSLayoutAttributeRight
                                                                 multiplier:1.
                                                                   constant:dist];
    [self.superview addConstraint:constraint];
    return constraint;
}

- (NSLayoutConstraint*) addRightSpaceToViewConstraint:(UIView*)view withDist:(int)dist
{
    self.translatesAutoresizingMaskIntoConstraints = NO;
    NSLayoutConstraint* constraint = [NSLayoutConstraint constraintWithItem:view
                                                                  attribute:NSLayoutAttributeLeft
                                                                  relatedBy:NSLayoutRelationEqual
                                                                     toItem:self
                                                                  attribute:NSLayoutAttributeRight
                                                                 multiplier:1.
                                                                   constant:dist];
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
        if (con.firstItem == self.superview && con.secondItem == self && con.firstAttribute == NSLayoutAttributeTop) return con;
    }
    return nil;
}

- (NSLayoutConstraint*) findBottomToSuperviewConstraint
{
    for (NSLayoutConstraint* con in self.superview.constraints)
    {
        if (con.firstItem == self && con.secondItem == self.superview && con.firstAttribute == NSLayoutAttributeBottom) return con;
    }
    return nil;
}

@end
