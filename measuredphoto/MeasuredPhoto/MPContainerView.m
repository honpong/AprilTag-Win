//
//  MPContainerView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/27/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPContainerView.h"
#import <RCCore/RCCore.h>

@implementation MPContainerView
@synthesize delegate;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        
    }
    return self;
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    [self applyRotationTransformation:orientation animated:animated];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [self removeConstraintsFromSuperview];
        [self addMatchSuperviewConstraints];
    }
    else
    {
        // simply change width and height while leaving constraints in place
        if (orientation == UIDeviceOrientationPortrait || orientation == UIDeviceOrientationPortraitUpsideDown)
        {
            [self modifyWidthContraint:320 andHeightConstraint:480];
        }
        else if (orientation == UIDeviceOrientationLandscapeLeft || orientation == UIDeviceOrientationLandscapeRight)
        {
            [self modifyWidthContraint:480 andHeightConstraint:320];
        }
    }
}

// pass all touch events to the delegate, which in this case is the augmented reality view
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
