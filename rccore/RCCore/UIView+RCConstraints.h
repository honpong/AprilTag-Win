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
- (void) addCenterInSuperviewConstraints;
- (NSLayoutConstraint*) addCenterXInSuperviewConstraints;
- (NSLayoutConstraint*) addCenterYInSuperviewConstraints;
- (void) addWidthConstraint:(CGFloat)width andHeightConstraint:(CGFloat)height;
- (NSLayoutConstraint*) addTopSpaceToSuperviewConstraint:(CGFloat)constant;
- (NSLayoutConstraint*) addBottomSpaceToSuperviewConstraint:(CGFloat)constant;
- (NSLayoutConstraint*) addLeadingSpaceToSuperviewConstraint:(CGFloat)constant;
- (NSLayoutConstraint*) addTrailingSpaceToSuperviewConstraint:(CGFloat)constant;

- (NSLayoutConstraint*) getWidthConstraint:(CGFloat)width;
- (NSLayoutConstraint*) getHeightConstraint:(CGFloat)height;

- (void) modifyWidthContraint:(float)width andHeightConstraint:(float)height;

- (NSLayoutConstraint*) findWidthConstraint;
- (NSLayoutConstraint*) findHeightConstraint;
- (NSLayoutConstraint*) findTopToSuperviewConstraint;

@end
