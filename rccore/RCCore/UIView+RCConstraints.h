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
- (void) addCenterXInSuperviewConstraints;
- (void) addCenterYInSuperviewConstraints;
- (void) addWidthConstraint:(CGFloat)width andHeightConstraint:(CGFloat)height;
- (void) addTopSpaceToSuperviewConstraint:(CGFloat)constant;
- (void) addBottomSpaceToSuperviewConstraint:(CGFloat)constant;
- (void) addLeadingSpaceToSuperviewConstraint:(CGFloat)constant;
- (void) addTrailingSpaceToSuperviewConstraint:(CGFloat)constant;

- (void) modifyWidthContraint:(float)width andHeightConstraint:(float)height;

- (NSLayoutConstraint*) findWidthConstraint;
- (NSLayoutConstraint*) findHeightConstraint;
- (NSLayoutConstraint*) findTopToSuperviewConstraint;

@end
