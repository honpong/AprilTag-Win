//
//  UIView+RCOrientationRotation.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (RCOrientationRotation)

- (void) rotateChildViews:(UIDeviceOrientation)orientation;
- (void) applyRotationTransformationAnimated:(UIDeviceOrientation)deviceOrientation;
/**
 @returns Returns nil if orientation is not portrait or landscape
 */
- (NSNumber*) getRotationInRadians:(UIDeviceOrientation)deviceOrientation;

@end
