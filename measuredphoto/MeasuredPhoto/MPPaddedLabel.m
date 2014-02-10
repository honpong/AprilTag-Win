//
//  MPPaddedLabel.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPPaddedLabel.h"
#import "UIView+MPCascadingRotation.h"

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
    [self applyRotationTransformation:orientation];
}

@end
