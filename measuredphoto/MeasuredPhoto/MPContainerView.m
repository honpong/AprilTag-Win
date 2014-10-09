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
{
    CGFloat widthPortrait;
    CGFloat heightPortrait;
}
@synthesize delegate;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        NSLayoutConstraint* widthConstraint = [self findWidthConstraint];
        NSLayoutConstraint* heightConstraint = [self findHeightConstraint];
        
        widthPortrait = widthConstraint.constant;
        heightPortrait = heightConstraint.constant;
    }
    return self;
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    if (orientation == UIDeviceOrientationPortrait || orientation == UIDeviceOrientationPortraitUpsideDown)
    {
        [self modifyWidthContraint:widthPortrait andHeightConstraint:heightPortrait];
    }
    else if (orientation == UIDeviceOrientationLandscapeLeft || orientation == UIDeviceOrientationLandscapeRight)
    {
        [self modifyWidthContraint:heightPortrait andHeightConstraint:widthPortrait];
    }
    
    [self setNeedsUpdateConstraints];
    
    [self setNeedsUpdateConstraints];
    [UIView animateWithDuration: .3
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [self applyRotationTransformation:orientation animated:NO];
                         [self layoutIfNeeded];
                     }
                     completion:nil];
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
