//
//  MPContainerView.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/27/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPContainerView.h"
#import <RCCore/RCCore.h>
#import "MPCapturePhoto.h"

@implementation MPContainerView
{
    CGFloat widthPortrait;
    CGFloat heightPortrait;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        NSLayoutConstraint* widthConstraint = [self findWidthConstraint];
        NSLayoutConstraint* heightConstraint = [self findHeightConstraint];
        
        widthPortrait = widthConstraint.constant;
        heightPortrait = heightConstraint.constant;
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleOrientationChange:)
                                                     name:MPUIOrientationDidChangeNotification
                                                   object:nil];
    }
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handleOrientationChange:(NSNotification*)notification
{
    if (notification.object)
    {
        MPOrientationChangeData* data = (MPOrientationChangeData*)notification.object;
        
        if (data.orientation == UIDeviceOrientationPortrait || data.orientation == UIDeviceOrientationPortraitUpsideDown)
        {
            [self modifyWidthContraint:widthPortrait andHeightConstraint:heightPortrait];
        }
        else if (data.orientation == UIDeviceOrientationLandscapeLeft || data.orientation == UIDeviceOrientationLandscapeRight)
        {
            [self modifyWidthContraint:heightPortrait andHeightConstraint:widthPortrait];
        }
            
        [UIView animateWithDuration: .3
                              delay: 0
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             [self applyRotationTransformation:data.orientation animated:NO];
                         }
                         completion:^(BOOL finished){
                             [self setNeedsUpdateConstraints];
                             [self setNeedsLayout];
                             [self layoutIfNeeded];
                         }];
    }
}

@end
