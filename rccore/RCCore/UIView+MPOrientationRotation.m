//
//  UIView+MPCascadingRotation.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "UIView+MPOrientationRotation.h"
#import "RCRotatingView.h"

@implementation UIView (MPOrientationRotation)

- (void) rotateChildViews:(UIDeviceOrientation)orientation
{
    for (id<RCRotatingView> subView in self.subviews)
    {
        if ([subView respondsToSelector:@selector(handleOrientationChange:)])
        {
            [subView handleOrientationChange:orientation];
        }
    }
}

- (void) applyRotationTransformationAnimated:(UIDeviceOrientation)deviceOrientation
{
    [UIView animateWithDuration: .5
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self applyRotationTransformation:deviceOrientation];
                     }
                     completion:nil];
}

- (void) applyRotationTransformation:(UIDeviceOrientation)deviceOrientation
{
    NSNumber* radians = [self getRotationInRadians:deviceOrientation];
    if (radians)
    {
        if (radians == 0)
            self.transform = CGAffineTransformIdentity;
        else
            self.transform = CGAffineTransformMakeRotation([radians floatValue]);
    }
    
}

- (NSNumber*) getRotationInRadians:(UIDeviceOrientation)deviceOrientation
{
    switch (deviceOrientation)
    {
        case UIDeviceOrientationPortrait: return @0;
        case UIDeviceOrientationPortraitUpsideDown: return @M_PI;
        case UIDeviceOrientationLandscapeLeft: return @M_PI_2;
        case UIDeviceOrientationLandscapeRight: return @-M_PI_2;
        default: return nil;
    }
}

@end
