//
//  MPCalibration2.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPCalibration2.h"
#import "MPCalibration3.h"
#import "MBProgressHUD.h"

@implementation MPCalibration2
{
    BOOL isCalibrating;
    MBProgressHUD *progressView;
    NSDate* startTime;
}
@synthesize button, messageLabel, videoPreview;

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
    SENSOR_FUSION.delegate = self;
    [VIDEO_MANAGER setupWithSession:SESSION_MANAGER.session];
    VIDEO_MANAGER.delegate = videoPreview;
}

- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    self.screenName = @"Calibration2";
    [SESSION_MANAGER startSession];
    [self handleOrientation:self.interfaceOrientation];
}

- (void) willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    [self handleOrientation:toInterfaceOrientation];
}

- (void) handleOrientation:(UIInterfaceOrientation)orientation
{
    // must be done on UI thread
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
    
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        switch (orientation) {
            case UIInterfaceOrientationPortrait:
                [SESSION_MANAGER setVideoOrientation:AVCaptureVideoOrientationPortrait];
                break;
            case UIInterfaceOrientationPortraitUpsideDown:
                [SESSION_MANAGER setVideoOrientation:AVCaptureVideoOrientationPortraitUpsideDown];
                break;
            case UIInterfaceOrientationLandscapeLeft:
                [SESSION_MANAGER setVideoOrientation:AVCaptureVideoOrientationLandscapeLeft];
                break;
            case UIInterfaceOrientationLandscapeRight:
                [SESSION_MANAGER setVideoOrientation:AVCaptureVideoOrientationLandscapeRight];
                break;
        }
    });
}

- (void) handlePause
{
    [self stopCalibration];
}

- (void) handleResume
{
    [SESSION_MANAGER startSession];
}

- (IBAction) handleButton:(id)sender
{
    [self startCalibration];
}

- (void) gotoNextScreen
{
    MPCalibration3* cal3 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration3"];
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
    DLog(@"SENSOR FUSION ERROR %i", error.code);
    [self startTimer];
}

- (void) startTimer
{
    startTime = [NSDate date];
}

- (void) startCalibration
{
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Hold the device steady and make sure the camera isn't blocked"];
    [self showProgressWithTitle:@"Calibrating"];
    
    SENSOR_FUSION.delegate = self;
    [SENSOR_FUSION startProcessingVideo];
    [VIDEO_MANAGER startVideoCapture];
        
    [self startTimer];
    
    isCalibrating = YES;
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        isCalibrating = NO;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
        [messageLabel setText:@"Hold the iPad steady in portrait orientation. Make sure the camera lens isn't blocked. Step 2 of 3."];
        [self hideProgress];
        [VIDEO_MANAGER stopVideoCapture];
        [SENSOR_FUSION stopProcessingVideo];
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
