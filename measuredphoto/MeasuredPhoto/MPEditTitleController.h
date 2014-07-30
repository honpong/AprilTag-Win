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
@property (weak, nonatomic) IBOutlet MPTitleTextBox *titleText;
@property (weak, nonatomic) IBOutlet UIButton *cancelButton;
@property (weak, nonatomic) IBOutlet UIImageView *photoView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *cancelButtonTrailingSpace;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *titleTextWidth;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *titleTextCenterX;
@property (weak, nonatomic) IBOutlet UIView *navBar;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *navBarTopSpace;

- (IBAction)handleCancelButton:(id)sender;

@end
