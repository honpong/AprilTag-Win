//
//  UIView+MPCascadingRotation.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+MPCascadingRotation.h"
#import "MPRotatingView.h"

@implementation UIView (MPCascadingRotation)

- (void) rotateChildViews:(UIDeviceOrientation)orientation
{
    for (id<MPRotatingView> subView in self.subviews)
    {
        if ([subView respondsToSelector:@selector(handleOrientationChange:)])
        {
            [subView handleOrientationChange:orientation];
        }
    }
}

- (void) applyRotationTransformation:(UIDeviceOrientation)deviceOrientation
{
    switch (deviceOrientation)
    {
        case UIDeviceOrientationPortrait:
        {
            self.transform = CGAffineTransformIdentity;
            break;
        }
        case UIDeviceOrientationPortraitUpsideDown:
        {
            self.transform = CGAffineTransformMakeRotation(M_PI);
            break;
        }
        case UIDeviceOrientationLandscapeLeft:
        {
            self.transform = CGAffineTransformMakeRotation(M_PI_2);
            break;
        }
        case UIDeviceOrientationLandscapeRight:
        {
            self.transform = CGAffineTransformMakeRotation(-M_PI_2);
            break;
        }
        default:
            break;
    }
}

@end
