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

- (IBAction)handleCancelButton:(id)sender;

@end
