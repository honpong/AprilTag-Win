//
//  MPEditTitleController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPDMeasuredPhoto+MPDMeasuredPhotoExt.h"
#import "MPTitleTextBox.h"

@interface MPEditTitleController : UIViewController <UITextFieldDelegate>

@property (nonatomic) MPDMeasuredPhoto* measuredPhoto;
@property (nonatomic) IBOutlet MPTitleTextBox *titleText;
@property (nonatomic) IBOutlet UIButton *cancelButton;
@property (nonatomic) IBOutlet UIImageView *photoView;
@property (nonatomic) IBOutlet NSLayoutConstraint *cancelButtonTrailingSpace;
@property (nonatomic) IBOutlet NSLayoutConstraint *titleTextWidth;
@property (nonatomic) IBOutlet NSLayoutConstraint *titleTextCenterX;
@property (nonatomic) IBOutlet UIView *navBar;
@property (nonatomic) IBOutlet NSLayoutConstraint *navBarTopSpace;
@property (nonatomic) UIInterfaceOrientationMask supportedUIOrientations;

- (IBAction)handleCancelButton:(id)sender;

@end
