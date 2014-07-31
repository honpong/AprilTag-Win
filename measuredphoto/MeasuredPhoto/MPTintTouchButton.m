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

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        self.imageView.highlightedImage = [self.imageView.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
        self.showsTouchWhenHighlighted = YES;
    }
    return self;
}

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self.imageView setHighlighted: YES];
    [super touchesBegan:touches withEvent:event];
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
    
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(highlightDelay * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self.imageView setHighlighted: NO];
    });
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
    
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(highlightDelay * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self.imageView setHighlighted: NO];
    });
}

@end
