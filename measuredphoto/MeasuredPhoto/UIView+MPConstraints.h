//
//  UIView+MPConstraints.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (MPConstraints)

- (void) addMatchSuperviewConstraints;
- (void) addCenterInSuperviewConstraints;
- (void) addWidthConstraint:(CGFloat)width andHeightConstraint:(CGFloat)height;

@end
