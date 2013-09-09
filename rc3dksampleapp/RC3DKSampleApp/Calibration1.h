//
//  RCCalibration1.h
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <RC3DK/RC3DK.h>

@protocol CalibrationDelegate

-(void)calibrationDidFinish;

@end

@interface Calibration1 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak) id<CalibrationDelegate> delegate;

+ (UIViewController *) instantiateViewControllerWithDelegate:(id)delegate;
- (IBAction)handleButton:(id)sender;

@end
