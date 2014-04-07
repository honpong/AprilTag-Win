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
/**
 @returns The AVCaptureDevice used to capture video.
 */
- (AVCaptureDevice*) getVideoDevice;
/**
 @returns Gets a object that sends video frames to it's delegate. This is where the video frames for the video preview views come from.
 */
- (id<RCVideoFrameProvider>) getVideoProvider;
/**
 Starts the AVCaptureSession that we're using to capture video.
 */
- (void) startVideoSession;
/**
 Stops the AVCaptureSession.
 */
- (void) stopVideoSession;
/**
 Called when all three calibration steps have been completed.
 */
- (void) calibrationDidFinish;
@optional
/**
 Called if a failure occurs during calibration. Not currently used.
 */
- (void) calibrationDidFail:(NSError*)error;
/**
 For notifying the delegate when calibration progresses from one step (view controller) to another
 */
- (void) calibrationScreenDidAppear:(NSString*)screenName;

@end

/**
 The calibration procedure consists of three distinct steps. Each step is handled by it's own view controller.
 The calibration view controllers have a delegate that gets passed from one view controller to the next. When
 the last step is completed, the calibrationDidFinish method is called on the delegate. See the 
 RCCalibrationDelegate protocol.
 */
@interface RCCalibration1 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) id<RCCalibrationDelegate> delegate;
@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;

+ (RCCalibration1 *) instantiateViewControllerWithDelegate:(id)delegate;
- (IBAction)handleButton:(id)sender;

@end
