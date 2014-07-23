//
//  MPTitleTextBox.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPTitleTextBox.h"
#import "CoreData+MagicalRecord.h"

@implementation MPTitleTextBox

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
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
    UIDeviceOrientation orientation;
    
    if (notification.object)
    {
        [((NSValue*)notification.object) getValue:&orientation];
//        [self applyRotationTransformation:orientation animated:YES];
        
        if (orientation == UIDeviceOrientationPortrait || orientation == UIDeviceOrientationPortraitUpsideDown)
        {
            [self expandTitleBox];
        }
        else
        {
            [self shrinkTitleBox];
        }
    }
}

- (void) shrinkTitleBox
{
    [UIView animateWithDuration: .5
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.placeholder = nil;
                         self.text = nil;
                         self.transform = CGAffineTransformMakeScale(.2, 1.);
                     }
                     completion:nil];
}

- (void) expandTitleBox
{
    [UIView animateWithDuration: .5
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.placeholder = @"Title";
                         self.text = self.measuredPhoto.name;
                         self.transform = CGAffineTransformMakeScale(1., 1.);
                     }
                     completion:nil];
}

- (void) setMeasuredPhoto:(MPDMeasuredPhoto *)measuredPhoto
{
    self.text = measuredPhoto.name;
}

@end
