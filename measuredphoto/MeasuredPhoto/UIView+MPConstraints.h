//
//  UIView+MPConstraints.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (MPConstraints)

- (void) removeConstraintsFromSuperview;
- (void) addMatchSuperviewConstraints;
- (void) addCenterInSuperviewConstraints;
- (void) addCenterXInSuperviewConstraints;
- (void) addCenterYInSuperviewConstraints;
- (void) addWidthConstraint:(CGFloat)width andHeightConstraint:(CGFloat)height;
- (void) modifyWidthContraint:(float)width andHeightConstraint:(float)height;

@end
