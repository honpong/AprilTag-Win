//
//  MPTitleTextBox.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPTitleTextBox.h"
#import "CoreData+MagicalRecord.h"
#import "MPOrientationChangeData.h"
#import "MPConstants.h"

@implementation MPTitleTextBox
{
    NSString* placeholderText;
    CGFloat originalWidth;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleOrientationChange:)
                                                     name:MPUIOrientationDidChangeNotification
                                                   object:nil];
        placeholderText = self.placeholder;
    }
    return self;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) setWidthConstraint:(NSLayoutConstraint *)widthConstraint
{
    _widthConstraint = widthConstraint;
    originalWidth = widthConstraint.constant;
}

- (void) handleOrientationChange:(NSNotification*)notification
{
    if (notification.object)
    {
        MPOrientationChangeData* data = (MPOrientationChangeData*)notification.object;
        
        if (UIDeviceOrientationIsPortrait(data.orientation))
        {
            [self expandTitleBox];
        }
        else if (UIDeviceOrientationIsLandscape(data.orientation))
        {
            [self shrinkTitleBox];
        }
    }
}

- (void) shrinkTitleBox
{
    self.placeholder = nil;
    self.textColor = [UIColor whiteColor];
    
    [self layoutIfNeeded]; // apple recommends calling before animation to ensure any pending animations finish
    
    [UIView animateWithDuration: .3
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.widthConstraint.constant = self.bounds.size.height;
                         [self setNeedsUpdateConstraints];
                         [self layoutIfNeeded];
                     }
                     completion:^(BOOL finished){
//                         self.hidden = YES;
                     }];
}

- (void) expandTitleBox
{
    self.hidden = NO;
    
    [self layoutIfNeeded]; // apple recommends calling before animation to ensure any pending animations finish
    
    [UIView animateWithDuration: .3
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.widthConstraint.constant = originalWidth;
                         [self setNeedsUpdateConstraints];
                         [self layoutIfNeeded];
                     }
                     completion:^(BOOL finished) {
                         self.placeholder = placeholderText;
                         self.textColor = [UIColor blackColor];
                     }];
}

- (void) setMeasuredPhoto:(MPDMeasuredPhoto *)measuredPhoto
{
    _measuredPhoto = measuredPhoto;
    self.text = measuredPhoto.name;
}

@end
