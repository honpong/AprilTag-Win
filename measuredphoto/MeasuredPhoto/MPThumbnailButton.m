//
//  MPThumbnailButton.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPThumbnailButton.h"
#import "UIView+MPConstraints.h"

@implementation MPThumbnailButton

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    NSArray *thumbnailH, *thumbnailV;
    UIButton* thumbnail = self;
    
    switch (orientation)
    {
        case UIDeviceOrientationPortrait:
        {
            thumbnailH = [NSLayoutConstraint constraintsWithVisualFormat:@"[thumbnail(50)]-25-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            thumbnailV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[thumbnail(50)]-25-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            break;
        }
        case UIDeviceOrientationPortraitUpsideDown:
        {
            thumbnailH = [NSLayoutConstraint constraintsWithVisualFormat:@"[thumbnail(50)]-25-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            thumbnailV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-25-[thumbnail(50)]" options:0 metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            break;
        }
        case UIDeviceOrientationLandscapeLeft:
        {
            thumbnailH = [NSLayoutConstraint constraintsWithVisualFormat:@"|-25-[thumbnail(50)]" options:0 metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            thumbnailV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[thumbnail(50)]-25-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            break;
        }
        case UIDeviceOrientationLandscapeRight:
        {
            thumbnailH = [NSLayoutConstraint constraintsWithVisualFormat:@"[thumbnail(50)]-25-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            thumbnailV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-25-[thumbnail(50)]" options:0 metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            break;
        }
        default:
            break;
    }

    if (thumbnailH && thumbnailV)
    {
        [self removeConstraintsFromSuperview];
        [self.superview addConstraints:thumbnailH];
        [self.superview addConstraints:thumbnailV];
    }
}

@end
