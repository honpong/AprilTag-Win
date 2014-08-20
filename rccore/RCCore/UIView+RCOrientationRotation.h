//
//  UIView+RCOrientationRotation.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (RCOrientationRotation)

- (void) rotateChildViews:(UIDeviceOrientation)orientation animated:(BOOL)animated;
- (void) applyRotationTransformation:(UIDeviceOrientation)deviceOrientation animated:(BOOL)animated;
/**
 @returns Returns nil if orientation is not portrait or landscape
 */
- (NSNumber*) getRotationInRadians:(UIDeviceOrientation)deviceOrientation;
/**
 @returns Returns only portrait right side up, or landscape right side up
 */
+ (UIImageOrientation) imageOrientationFromDeviceOrientation:(UIDeviceOrientation)deviceOrientation;
+ (UIDeviceOrientation) deviceOrientationFromUIOrientation:(UIInterfaceOrientation)uiOrientation;

@end
