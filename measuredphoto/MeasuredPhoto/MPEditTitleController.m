//
//  MPEditTitleController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPEditTitleController.h"
#import "CoreData+MagicalRecord.h"

@interface MPEditTitleController ()

@end

@implementation MPEditTitleController
{
    BOOL isCanceled;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.titleText.delegate = self;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientationChange:)
                                                 name:MPUIOrientationDidChangeNotification
                                               object:nil];
}

- (void) viewDidAppear:(BOOL)animated
{
    isCanceled = NO;
    
    [self.titleText becomeFirstResponder];
    
    UIImage* photo = [UIImage imageWithContentsOfFile:self.measuredPhoto.imageFileName];
    [self.photoView setImage: photo];
    self.titleText.text = self.measuredPhoto.name;
    
    [self growTitleTextBox];
}

- (void) growTitleTextBox
{
    NSArray* constraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|-[titleText]-80-|" options:0 metrics:nil views:@{ @"titleText": self.titleText }];
    
    if (UIInterfaceOrientationIsPortrait(self.interfaceOrientation))
    {
        NSString* placeholder = self.titleText.placeholder;
        self.titleText.placeholder = nil; // hide placeholder text while animating because placeholder animates weirdly
        self.titleText.text = nil;
        
        [UIView animateWithDuration: .5
                              delay: .2
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             [self.titleText removeConstraint:self.titleTextWidth];
                             [self.navBar removeConstraint:self.titleTextCenterX];
                             [self.navBar addConstraints:constraints];
                             
                             [self.titleText setNeedsUpdateConstraints];
                             [self.titleText layoutIfNeeded];
                             [self.navBar setNeedsUpdateConstraints];
                             [self.navBar layoutIfNeeded];
                         }
                         completion:^(BOOL finished){
                             self.titleText.placeholder = placeholder;
                             self.titleText.text = self.measuredPhoto.name;
                         }];
    }
    else
    {
        [self.titleText removeConstraint:self.titleTextWidth];
        [self.navBar removeConstraint:self.titleTextCenterX];
        [self.navBar addConstraints:constraints];
        
        self.navBarTopSpace.constant = -self.navBar.bounds.size.height;
        [self.view layoutIfNeeded];
        
        [UIView animateWithDuration: .25
                              delay: 0
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             self.navBarTopSpace.constant = 0;
                             
                             [self.view setNeedsUpdateConstraints];
                             [self.view layoutIfNeeded];
                         }
                         completion:^(BOOL finished){
                             
                         }];
    }
}

- (IBAction)handleCancelButton:(id)sender
{
    isCanceled = YES;
    [self.presentingViewController dismissViewControllerAnimated:NO completion:nil];
}

- (void) setMeasuredPhoto:(MPDMeasuredPhoto *)measuredPhoto
{
    _measuredPhoto = measuredPhoto;
}

- (void) handleOrientationChange:(NSNotification*)notification
{
    UIDeviceOrientation orientation;
    
    if (notification.object)
    {
        [((NSValue*)notification.object) getValue:&orientation];
        
        if (orientation == UIDeviceOrientationPortrait || orientation == UIDeviceOrientationPortraitUpsideDown)
        {
//            [self.titleText resignFirstResponder];
        }
    }
}

#pragma mark - UITextFieldDelegate

- (void) textFieldDidEndEditing:(UITextField *)textField
{
    if (!isCanceled)
    {
        self.measuredPhoto.name = textField.text;
        
        [CONTEXT MR_saveToPersistentStoreWithCompletion:^(BOOL success, NSError *error) {
            if (success) {
                DLog(@"Saved CoreData context.");
            } else if (error) {
                DLog(@"Error saving context: %@", error.description);
            }
        }];
        
        [self.presentingViewController dismissViewControllerAnimated:NO completion:nil];
    }
}

- (BOOL) textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

@end
