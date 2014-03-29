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

- (void) calibrationDidFinish;
- (void) calibrationDidFail:(NSError*)error;
- (void) calibrationScreenDidAppear:(NSString*)screenName;

@end

@protocol RCVideoFrameProvider <NSObject>

@property id<RCVideoFrameDelegate> delegate;

@end

@interface RCCalibration1 : UIViewController <RCSensorFusionDelegate>

@property (weak, nonatomic) id<RCCalibrationDelegate> delegate;
@property (weak, nonatomic) IBOutlet UIButton *button;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (nonatomic) AVCaptureDevice* videoDevice;
@property (nonatomic) id<RCVideoFrameProvider> videoProvider;

+ (RCCalibration1 *) instantiateViewControllerWithDelegate:(id)delegate;
- (IBAction)handleButton:(id)sender;

@end
