//
//  MPContainerView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/27/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPContainerView.h"
#import "UIView+MPCascadingRotation.h"
#import "UIView+MPConstraints.h"

@implementation MPContainerView
@synthesize delegate;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        
    }
    return self;
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    [self applyRotationTransformation:orientation];
    [self removeConstraintsFromSuperview];
    [self addMatchSuperviewConstraints];
}

- (void) touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ([delegate respondsToSelector:@selector(touchesBegan:withEvent:)]) [delegate touchesBegan:touches withEvent:event];
}

- (void) touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ([delegate respondsToSelector:@selector(touchesMoved:withEvent:)]) [delegate touchesMoved:touches withEvent:event];
}

- (void) touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ([delegate respondsToSelector:@selector(touchesEnded:withEvent:)]) [delegate touchesEnded:touches withEvent:event];
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    if ([delegate respondsToSelector:@selector(touchesCancelled:withEvent:)]) [delegate touchesCancelled:touches withEvent:event];
}

@end
