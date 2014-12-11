//
//  MPPaddedLabel.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/22/13.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "CATPaddedLabel.h"

static UIEdgeInsets insets;

@implementation CATPaddedLabel

+ (void) initialize
{
    insets = UIEdgeInsetsMake(5, 15, 5, 15);
}

- (void) drawTextInRect:(CGRect)rect
{
    [super drawTextInRect:UIEdgeInsetsInsetRect(rect, insets)];
    [self invalidateIntrinsicContentSize];
    [self setNeedsLayout];
}

@end
