//
//  MPMessageBox.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPMessageBox.h"

@implementation MPMessageBox

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
}

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    NSArray *horzContraints, *vertConstraints;
    NSLayoutConstraint* horzCenteringContraint;
    float distFromBottom = 0;
    
    switch (orientation)
    {
        case UIDeviceOrientationPortrait:
        {
            horzContraints = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"[self(384)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            vertConstraints = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"V:[self(80)]-(%0.0f)-|", distFromBottom]
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
            break;
        }
        case UIDeviceOrientationPortraitUpsideDown:
        {
            horzContraints = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"[self(384)]"
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            vertConstraints = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"V:|-(%0.0f)-[self(80)]", distFromBottom]
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
            break;
        }
        case UIDeviceOrientationLandscapeLeft:
        {
            horzContraints = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"|-(%0.0f)-[self(80)]", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            vertConstraints = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"V:[self(384)]"
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
            break;
        }
        case UIDeviceOrientationLandscapeRight:
        {
            horzContraints = [NSLayoutConstraint
                         constraintsWithVisualFormat:[NSString stringWithFormat:@"[self(80)]-(%0.0f)-|", distFromBottom]
                         options:NSLayoutFormatDirectionLeadingToTrailing
                         metrics:nil
                         views:NSDictionaryOfVariableBindings(self)];
            vertConstraints = [NSLayoutConstraint
                         constraintsWithVisualFormat:@"V:[self(384)]"
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
            break;
        }
        default:
            break;
    }
    
    if (horzContraints && vertConstraints && horzCenteringContraint)
    {
        [self.superview addConstraint:horzCenteringContraint];
        [self.superview addConstraints:horzContraints];
        [self.superview addConstraints:vertConstraints];
    }
}

@end
