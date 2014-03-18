//
//  MPCalibrationVCViewController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPViewController.h"
#import <RCCore/RCCore.h>
#import "MBProgressHUD.h"
#import "MPCapturePhoto.h"

@interface MPCalibration1 : MPViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;

- (IBAction)handleButton:(id)sender;

@end
