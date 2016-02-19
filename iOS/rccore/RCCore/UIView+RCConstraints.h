//
//  UIView+RCConstraints.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (RCConstraints)

- (void) removeConstraintsFromSuperview;

- (void) addMatchSuperviewConstraints;
- (void) addMatchSuperviewConstraints:(CGFloat)spacing;

- (void) addCenterInSuperviewConstraints;
- (NSLayoutConstraint*) addCenterXInSuperviewConstraints;
- (NSLayoutConstraint*) addCenterXInSuperviewConstraints:(CGFloat)offset;
- (NSLayoutConstraint*) addCenterYInSuperviewConstraints;
- (NSLayoutConstraint*) addCenterYInSuperviewConstraints:(CGFloat)offset;

- (void) addWidthConstraint:(CGFloat)width andHeightConstraint:(CGFloat)height;
- (NSLayoutConstraint*) addWidthConstraint:(CGFloat)width;
- (NSLayoutConstraint*) addHeightConstraint:(CGFloat)height;

- (void) addMatchWidthToSuperviewConstraints;
- (void) addMatchWidthToSuperviewConstraints:(CGFloat)spacing;
- (void) addMatchHeightToSuperviewContraints;
- (void) addMatchHeightToSuperviewContraints:(CGFloat)spacing;

- (NSLayoutConstraint*) addTopSpaceToSuperviewConstraint:(CGFloat)constant;
- (NSLayoutConstraint*) addBottomSpaceToSuperviewConstraint:(CGFloat)constant;
- (NSLayoutConstraint*) addLeftSpaceToSuperviewConstraint:(CGFloat)constant;
- (NSLayoutConstraint*) addRightSpaceToSuperviewConstraint:(CGFloat)constant;

- (NSLayoutConstraint*) addTopSpaceToViewConstraint:(UIView*)view withDist:(int)dist;
- (NSLayoutConstraint*) addBottomSpaceToViewConstraint:(UIView*)view withDist:(int)dist;
- (NSLayoutConstraint*) addLeftSpaceToViewConstraint:(UIView*)view withDist:(int)dist;
- (NSLayoutConstraint*) addRightSpaceToViewConstraint:(UIView*)view withDist:(int)dist;

- (NSLayoutConstraint*) getWidthConstraint:(CGFloat)width;
- (NSLayoutConstraint*) getHeightConstraint:(CGFloat)height;

- (void) modifyWidthContraint:(float)width andHeightConstraint:(float)height;

- (NSLayoutConstraint*) findWidthConstraint;
- (NSLayoutConstraint*) findHeightConstraint;
- (NSLayoutConstraint*) findTopToSuperviewConstraint;
- (NSLayoutConstraint*) findBottomToSuperviewConstraint;

@end
