//
//  RCCalibration3.m
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration3.h"

@interface RCCalibration3 ()

@end

@implementation RCCalibration3
{
    BOOL isCalibrating;
    bool steadyDone;
    MBProgressHUD *progressView;
    NSDate* startTime;
    RCSensorFusion* sensorFusion;
}
@synthesize button, messageLabel, videoPreview;

- (void) viewDidLoad
{
    [super viewDidLoad];
	
    isCalibrating = NO;
    steadyDone = NO;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientation)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
    
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
    [self.sensorDelegate getVideoProvider].delegate = self.videoPreview;
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.calibrationDelegate calibrationScreenDidAppear: @"Calibration3"];
    [super viewDidAppear:animated];
    [self handleResume];
    [videoPreview setVideoOrientation:AVCaptureVideoOrientationLandscapeRight];
    [self handleOrientation];
    [self handleResume];
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscapeRight;
}

- (void) handleOrientation
{
    // must be done on UI thread
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    if (orientation == UIDeviceOrientationLandscapeLeft)
    {
        button.enabled = YES;
        [button setTitle:@"Tap here to begin calibration" forState:UIControlStateNormal];
    }
    else
    {
        button.enabled = NO;
        [button setTitle:@"Hold in landscape orientation" forState:UIControlStateNormal];
    }
}

- (void) handlePause
{
    [self stopCalibration];
    [self.sensorDelegate stopAllSensors];
}

- (void) handleResume
{
    //We need video data whenever the view is active for the preview window
    [self.sensorDelegate startAllSensors];
}

- (IBAction) handleButton:(id)sender
{
    [self startCalibration];
}

- (void) gotoNextScreen
{
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationDidFinish)]) [self.calibrationDelegate calibrationDidFinish];
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    if (isCalibrating && steadyDone)
    {
        if (!startTime)
            [self startTimer];
        
        float progress = .5 - [startTime timeIntervalSinceNow] / 4.; // 2 seconds steady followed by 2 seconds of data
        
        if (progress < 1.)
        {
            [self updateProgressView:progress];
        }
        else
        {
            [self finishCalibration];
        }
    }
    if (data.sampleBuffer) [videoPreview displaySampleBuffer:data.sampleBuffer];
}

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if (isCalibrating && !steadyDone)
    {
        if (status.runState == RCSensorFusionRunStateRunning)
        {
            steadyDone = true;
        }
        else
        {
            [self updateProgressView:status.progress / 2.];
        }
    }
    
    if ([status.error isKindOfClass:[RCSensorFusionError class]])
    {
        NSLog(@"SENSOR FUSION ERROR %li", (long)status.error.code);
        startTime = nil;
        steadyDone = false;
        [sensorFusion stopSensorFusion];
        [sensorFusion startSensorFusionWithDevice:[self.sensorDelegate getVideoDevice]];
    }
    else if ([status.error isKindOfClass:[RCLicenseError class]])
    {
        [self stopCalibration];
        
        NSString * message = [NSString stringWithFormat:@"There was a problem validating your license. The license error code is: %li.", (long)status.error.code];
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"RC3DK License Error"
                                                        message:message
                                                       delegate:self
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        [alert show];
    }
}

- (void) startTimer
{
    startTime = [NSDate date];
}

- (void) startCalibration
{
    LOGME
    
    isCalibrating = YES;
    steadyDone = NO;
    
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Hold the device steady and make sure the camera isn't blocked"];
    [self showProgressViewWithTitle:@"Calibrating"];
    
    [sensorFusion startSensorFusionWithDevice:[self.sensorDelegate getVideoDevice]];
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        LOGME
        isCalibrating = NO;
        steadyDone = NO;
        [button setTitle:@"Tap here to begin calibration" forState:UIControlStateNormal];
        [messageLabel setText:@"Hold the iPad steady in landscape orientation. Make sure the camera lens isn't blocked. Step 3 of 3."];
        [self hideProgressView];
        startTime = nil;
        [sensorFusion stopSensorFusion];
    }
}

- (void) finishCalibration
{
    [self stopCalibration];
    [self gotoNextScreen];
}

- (void)showProgressViewWithTitle:(NSString*)title
{
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.view addSubview:progressView];
    progressView.labelText = title;
    [progressView show:YES];
}

- (void)hideProgressView
{
    [progressView hide:YES];
}

- (void)updateProgressView:(float)progress
{
    [progressView setProgress:progress];
}

@end
