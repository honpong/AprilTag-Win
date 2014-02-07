//
//  MPPaddedLabel.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPPaddedLabel.h"

@implementation MPPaddedLabel
{
    UIEdgeInsets insets;
}

- (id)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self initialize];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [self initialize];
    }
    return self;
}

- (void) initialize
{
    insets = UIEdgeInsetsMake(5, 7, 5, 7);
}

- (void) drawTextInRect:(CGRect)rect
{
    [super drawTextInRect:UIEdgeInsetsInsetRect(rect, insets)];
    [self invalidateIntrinsicContentSize];
    [self setNeedsLayout];
}

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
