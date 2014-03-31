//
//  MPCalibrationVCViewController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <RC3DK/RC3DK.h>
#import "MBProgressHUD.h"

@protocol RCCalibrationDelegate <NSObject>

@required
- (AVCaptureDevice*) getVideoDevice;
- (id<RCVideoFrameProvider>) getVideoProvider;
- (void) startVideoSession;
- (void) stopVideoSession;
- (void) calibrationDidFinish;
@optional
- (void) calibrationDidFail:(NSError*)error;
- (void) calibrationScreenDidAppear:(NSString*)screenName;

@end

@interface RCCalibration1 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) id<RCCalibrationDelegate> delegate;
@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;

+ (RCCalibration1 *) instantiateViewControllerWithDelegate:(id)delegate;
- (IBAction)handleButton:(id)sender;

@end
