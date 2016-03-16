//
//  MPEditTitleController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPEditTitleController.h"
#import "CoreData+MagicalRecord.h"
#import "MPFadeTransitionDelegate.h"
#import "MPConstants.h"
#import <RCCore/RCDebugLog.h>

@interface MPEditTitleController ()

@end

@implementation MPEditTitleController
{
    BOOL isCanceled;
    MPFadeTransitionDelegate* transitionDelegate;
    NSArray* constraintsTitleBoxFullWidth;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        self.supportedUIOrientations = UIInterfaceOrientationMaskAll;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.titleText.delegate = self;
    
    transitionDelegate = [MPFadeTransitionDelegate new];
    self.transitioningDelegate = transitionDelegate;
    
    constraintsTitleBoxFullWidth = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|-[titleText]-80-|" options:0 metrics:nil views:@{ @"titleText": self.titleText }];
}

- (void)viewWillAppear:(BOOL)animated
{
    if (UIInterfaceOrientationIsPortrait(self.interfaceOrientation))
    {
        self.navBarTopSpace.constant = 0;
    }
    else
    {
        self.navBarTopSpace.constant = -self.navBar.bounds.size.height;
    }
    [self.navBar.superview setNeedsUpdateConstraints];
    [self.navBar.superview layoutIfNeeded];
    
    [self setupPhotoConstraints];
}

- (void) viewDidAppear:(BOOL)animated
{
    isCanceled = NO;
    [self.titleText becomeFirstResponder];
    [self animateTitleTextBox];
}

- (void)viewDidDisappear:(BOOL)animated
{
    [self resetTitleBoxLayout];
}

- (NSUInteger) supportedInterfaceOrientations
{
    return self.supportedUIOrientations;
}

- (void) willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    [self setupPhotoConstraints];
}

- (void) setupPhotoConstraints
{
    BOOL isPhotoLandscape = self.photoView.image.size.width > self.photoView.image.size.height;
    
    if (isPhotoLandscape)
        [self padPhotoTopBottom];
    else
        [self padPhotoLeftRight];
}

- (void) padPhotoTopBottom
{
    self.imageViewTopSpace.constant = 33;
    self.imageViewBottomSpace.constant = 33;
    self.imageViewLeftSpace.constant = 0;
    self.imageViewRightSpace.constant = 0;
    
    [self.photoView.superview setNeedsUpdateConstraints];
    [self.photoView.superview layoutIfNeeded];
}

- (void) padPhotoLeftRight
{
    self.imageViewTopSpace.constant = 0;
    self.imageViewBottomSpace.constant = 0;
    self.imageViewLeftSpace.constant = 33;
    self.imageViewRightSpace.constant = 33;
    
    [self.photoView.superview setNeedsUpdateConstraints];
    [self.photoView.superview layoutIfNeeded];
}

- (void) animateTitleTextBox
{
    if (UIInterfaceOrientationIsPortrait(self.interfaceOrientation))
    {
        NSString* placeholder = self.titleText.placeholder;
        self.titleText.placeholder = nil; // hide placeholder text while animating because placeholder animates weirdly
        self.titleText.text = nil;
        
        [UIView animateWithDuration: .3
                              delay: .2
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             [self.titleText removeConstraint:self.titleTextWidth];
                             [self.navBar removeConstraint:self.titleTextCenterX];
                             [self.navBar addConstraints:constraintsTitleBoxFullWidth];
                             
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
        [self.navBar addConstraints:constraintsTitleBoxFullWidth];
        
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

- (void) resetTitleBoxLayout
{
    [self.navBar removeConstraints:constraintsTitleBoxFullWidth];
    [self.navBar addConstraint:self.titleTextCenterX];
    [self.titleText addConstraint:self.titleTextWidth];
    
    [self.navBar setNeedsUpdateConstraints];
    [self.navBar layoutIfNeeded];
    [self.titleText setNeedsUpdateConstraints];
    [self.titleText layoutIfNeeded];
    [self.view setNeedsUpdateConstraints];
    [self.view layoutIfNeeded];
}

- (IBAction)handleCancelButton:(id)sender
{
    isCanceled = YES;
    [self.titleText resignFirstResponder];
    [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

- (void) setMeasuredPhoto:(MPDMeasuredPhoto *)measuredPhoto
{
    _measuredPhoto = measuredPhoto;
    
    UIImage* photo = [UIImage imageWithContentsOfFile:self.measuredPhoto.imageFileName];
    [self.photoView setImage: photo];
    self.titleText.text = self.measuredPhoto.name;
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
        
        [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    }
}

- (BOOL) textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

@end
