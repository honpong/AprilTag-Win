//
//  MPMessageBox.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPMessageBox.h"
#import "UIView+MPConstraints.h"
#import "UIView+MPCascadingRotation.h"

@implementation MPMessageBox
{
    float heightOriginal;
    float widthOriginal;
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
    self.layer.cornerRadius = 20.;
    widthOriginal = self.bounds.size.width;
    heightOriginal = self.bounds.size.height;
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    NSArray *horzContraints, *vertConstraints;
    NSLayoutConstraint* horzCenteringContraint;
    
    if (orientation == UIDeviceOrientationPortrait || orientation == UIDeviceOrientationPortraitUpsideDown)
    {
        horzContraints = [NSLayoutConstraint
                          constraintsWithVisualFormat:[NSString stringWithFormat:@"[self(%0.0f)]", widthOriginal]
                          options:NSLayoutFormatDirectionLeadingToTrailing
                          metrics:nil
                          views:NSDictionaryOfVariableBindings(self)];
        horzCenteringContraint = [NSLayoutConstraint
                                  constraintWithItem:self.superview
                                  attribute:NSLayoutAttributeCenterX
                                  relatedBy:NSLayoutRelationEqual
                                  toItem:self
                                  attribute:NSLayoutAttributeCenterX
                                  multiplier:1
                                  constant:0];
        
        if (orientation == UIDeviceOrientationPortrait)
        {
            
            vertConstraints = [NSLayoutConstraint
                               constraintsWithVisualFormat:[NSString stringWithFormat:@"V:[self(%0.0f)]-|", heightOriginal]
                               options:NSLayoutFormatDirectionLeadingToTrailing
                               metrics:nil
                               views:NSDictionaryOfVariableBindings(self)];
        }
        else
        {
            vertConstraints = [NSLayoutConstraint
                               constraintsWithVisualFormat:[NSString stringWithFormat:@"V:|-[self(%0.0f)]", heightOriginal]
                               options:NSLayoutFormatDirectionLeadingToTrailing
                               metrics:nil
                               views:NSDictionaryOfVariableBindings(self)];
        }
    }
    else if (orientation == UIDeviceOrientationLandscapeLeft || orientation == UIDeviceOrientationLandscapeRight)
    {
        vertConstraints = [NSLayoutConstraint
                           constraintsWithVisualFormat:[NSString stringWithFormat:@"V:[self(%0.0f)]", widthOriginal]
                           options:NSLayoutFormatDirectionLeadingToTrailing
                           metrics:nil
                           views:NSDictionaryOfVariableBindings(self)];
        horzCenteringContraint = [NSLayoutConstraint
                                  constraintWithItem:self.superview
                                  attribute:NSLayoutAttributeCenterY
                                  relatedBy:NSLayoutRelationEqual
                                  toItem:self
                                  attribute:NSLayoutAttributeCenterY
                                  multiplier:1
                                  constant:0];
        
        if (orientation == UIDeviceOrientationLandscapeLeft)
        {
            horzContraints = [NSLayoutConstraint
                              constraintsWithVisualFormat:[NSString stringWithFormat:@"|-[self(%0.0f)]", heightOriginal]
                              options:NSLayoutFormatDirectionLeadingToTrailing
                              metrics:nil
                              views:NSDictionaryOfVariableBindings(self)];
        }
        else
        {
            horzContraints = [NSLayoutConstraint
                              constraintsWithVisualFormat:[NSString stringWithFormat:@"[self(%0.0f)]-|", heightOriginal]
                              options:NSLayoutFormatDirectionLeadingToTrailing
                              metrics:nil
                              views:NSDictionaryOfVariableBindings(self)];
        }
    }
    
    if (horzContraints && vertConstraints && horzCenteringContraint)
    {
        [self removeConstraintsFromSuperview];
        [self applyRotationTransformation:orientation];
        [self.superview addConstraint:horzCenteringContraint];
        [self.superview addConstraints:horzContraints];
        [self.superview addConstraints:vertConstraints];
    }
}

@end
