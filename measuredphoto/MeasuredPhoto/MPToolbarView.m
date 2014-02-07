//
//  MPToolbarView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/6/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPToolbarView.h"
#import "UIView+MPCascadingRotation.h"

@implementation MPToolbarView

- (void) handleOrientationChange:(UIDeviceOrientation)orientation
{
    if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPad) return;
    
    NSArray *toolbarH, *toolbarV;
    UIView* toolbar = self;
    
    switch (orientation)
    {
        case UIDeviceOrientationPortrait:
        {
            toolbarH = [NSLayoutConstraint constraintsWithVisualFormat:@"[toolbar(100)]-0-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            toolbarV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar]-0-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            break;
        }
        case UIDeviceOrientationPortraitUpsideDown:
        {
            toolbarH = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar(100)]" options:0 metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            toolbarV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar]-0-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            break;
        }
        case UIDeviceOrientationLandscapeLeft:
        {
            toolbarH = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar]-0-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            toolbarV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[toolbar(100)]-0-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            break;
        }
        case UIDeviceOrientationLandscapeRight:
        {
            toolbarH = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar]-0-|" options:0 metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            toolbarV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar(100)]" options:0 metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            break;
        }
        default:
            break;
    }
    
    if (toolbarH && toolbarV)
    {
        for (NSLayoutConstraint *con in self.superview.constraints)
        {
            if (con.firstItem == self || con.secondItem == self)
            {
                [self.superview removeConstraint:con];
            }
        }
        
        [self.superview addConstraints:toolbarH];
        [self.superview addConstraints:toolbarV];
    }
    
    [self rotateChildViews:orientation];
}

@end
