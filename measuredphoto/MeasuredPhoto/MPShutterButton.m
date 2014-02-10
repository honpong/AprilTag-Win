//
//  MPShutterButtonView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPShutterButton.h"

@implementation MPShutterButton

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    switch (orientation)
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
