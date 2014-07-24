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
{
    NSString* placeholderText;
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

- (void) handleOrientationChange:(NSNotification*)notification
{
    UIDeviceOrientation orientation;
    
    if (notification.object)
    {
        [((NSValue*)notification.object) getValue:&orientation];
        
        if (orientation == UIDeviceOrientationPortrait || orientation == UIDeviceOrientationPortraitUpsideDown)
        {
            [self expandTitleBox];
        }
        else if (orientation == UIDeviceOrientationLandscapeLeft || orientation == UIDeviceOrientationLandscapeRight)
        {
            [self shrinkTitleBox];
        }
    }
}

- (void) shrinkTitleBox
{
    self.placeholder = nil;
    self.textColor = [UIColor whiteColor];
    
    [UIView animateWithDuration: .3
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.transform = CGAffineTransformMakeScale(.2, 1.);
                     }
                     completion:^(BOOL finished){
                         self.hidden = YES;
                     }];
}

- (void) expandTitleBox
{
    self.hidden = NO;
    
    [UIView animateWithDuration: .3
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.transform = CGAffineTransformMakeScale(1., 1.);
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
