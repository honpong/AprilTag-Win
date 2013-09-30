//
//  CalibrationStep2.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <RCCore/RCVideoManager.h>
#import "CalibrationStep2.h"
#import "CalibrationStep3.h"
#import "MBProgressHUD.h"

@implementation CalibrationStep2
{
    BOOL isCalibrating;
    MBProgressHUD *progressView;
    NSDate* startTime;
}
@synthesize button, messageLabel;

- (void) viewDidLoad
{
    [super viewDidLoad];
	
    isCalibrating = NO;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    [RCSensorFusion sharedInstance].delegate = self;
    [[RCVideoManager sharedInstance] setupWithSession:[RCAVSessionManager sharedInstance].session];
}

- (void) viewDidAppear:(BOOL)animated
{
    [[RCAVSessionManager sharedInstance] startSession];
    [self handleOrientation:self.interfaceOrientation];
}

- (void) willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    [self handleOrientation:toInterfaceOrientation];
}

- (void) handleOrientation:(UIInterfaceOrientation)orientation
{
    if (orientation == UIInterfaceOrientationPortrait || orientation == UIInterfaceOrientationPortraitUpsideDown)
    {
        button.enabled = YES;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
    }
    else
    {
        button.enabled = NO;
        [button setTitle:@"Rotate to portrait" forState:UIControlStateNormal];
    }
}

- (void) handlePause
{
    [self stopCalibration];
}

- (void) handleResume
{
    [[RCAVSessionManager sharedInstance] startSession];
}

- (IBAction) handleButton:(id)sender
{
    [self startCalibration];
}

- (void) gotoNextScreen
{
    CalibrationStep3* cal3 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration3"];
    cal3.delegate = self.delegate;
    [self presentViewController:cal3 animated:YES completion:nil];
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (isCalibrating)
    {
        float progress = -[startTime timeIntervalSinceNow] / 5.; // 5 seconds
        
        if (progress < 1.)
        {
            [self updateProgress:progress];
        }
        else
        {
            [self finishCalibration];
        }
    }
}

- (void) sensorFusionError:(NSError*)error
{
    NSLog(@"SENSOR FUSION ERROR %i", error.code);
    [self startTimer];
}

- (void) startTimer
{
    startTime = [NSDate date];
}

- (void) startCalibration
{
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Hold the device steady"];
    [self showProgressWithTitle:@"Calibrating"];
    
    [RCSensorFusion sharedInstance].delegate = self;
    [[RCSensorFusion sharedInstance] startProcessingVideo];
    [[RCVideoManager sharedInstance] startVideoCapture];
        
    [self startTimer];
    
    isCalibrating = YES;
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        isCalibrating = NO;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
        [messageLabel setText:@"Hold the iPad steady in portrait orientation. Step 2 of 3."];
        [self hideProgress];
        [[RCVideoManager sharedInstance] stopVideoCapture];
        [[RCSensorFusion sharedInstance] stopProcessingVideo];
    }
}

- (void) finishCalibration
{
    [self stopCalibration];
    [self gotoNextScreen];
}

- (void)showProgressWithTitle:(NSString*)title
{
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.view addSubview:progressView];
    progressView.labelText = title;
    [progressView show:YES];
}

- (void)hideProgress
{
    [progressView hide:YES];
}

- (void)updateProgress:(float)progress
{
    [progressView setProgress:progress];
}


@end
