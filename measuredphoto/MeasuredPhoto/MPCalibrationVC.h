//
//  MPCalibrationVCViewController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPViewController.h"
#import <RC3DK/RC3DK.h>
#import "MBProgressHUD.h"

@protocol MPCalibrationDelegate <NSObject>

- (void) calibrationDidComplete;

@end

@interface MPCalibrationVC : MPViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) id<MPCalibrationDelegate> delegate;
@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;

- (IBAction)handleButton:(id)sender;

@end
