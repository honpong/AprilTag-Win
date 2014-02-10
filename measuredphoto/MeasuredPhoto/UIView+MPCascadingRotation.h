//
//  UIView+MPCascadingRotation.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (MPCascadingRotation)

- (void) rotateChildViews:(UIDeviceOrientation)orientation;
- (void) applyRotationTransformation:(UIDeviceOrientation)deviceOrientation;

@end
