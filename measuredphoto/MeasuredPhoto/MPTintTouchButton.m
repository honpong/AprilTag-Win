//
//  MPTintTouchButton.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/31/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPTintTouchButton.h"

static const NSTimeInterval highlightDelay = .3;

@implementation MPTintTouchButton
{
    UIColor* originalTextColor;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        self.imageView.highlightedImage = [self.imageView.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
        self.showsTouchWhenHighlighted = YES;
        
        originalTextColor = self.titleLabel.textColor;
    }
    return self;
}

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];

    [self.imageView setHighlighted: YES];
    
    originalTextColor = self.titleLabel.textColor;
    self.titleLabel.textColor = self.tintColor;
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
    [self revertButtonState];
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
    [self revertButtonState];
}

- (void) revertButtonState
{
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(highlightDelay * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self.imageView setHighlighted: NO];
        self.titleLabel.textColor = originalTextColor;
    });
}

@end
