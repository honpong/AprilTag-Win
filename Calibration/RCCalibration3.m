//
//  MPCalibration3.m
//  MeasuredPhoto
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
    MBProgressHUD *progressView;
    NSDate* startTime;
    RCSensorFusion* sensorFusion;
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
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientation)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
    
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
    [self.delegate getVideoProvider].delegate = self.videoPreview;
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.delegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.delegate calibrationScreenDidAppear: @"Calibration3"];
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
    [self.delegate stopVideoSession];
}

- (void) handleResume
{
    [self.delegate startVideoSession];
}

- (IBAction) handleButton:(id)sender
{
    [self startCalibration];
}

- (void) gotoNextScreen
{
    if ([self.delegate respondsToSelector:@selector(calibrationDidFinish)]) [self.delegate calibrationDidFinish];
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (isCalibrating && [sensorFusion isProcessingVideo])
    {
        if (!startTime)
            [self startTimer];

        float progress = -[startTime timeIntervalSinceNow] / 5.; // 5 seconds
        
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
    if(status.errorCode != RCSensorFusionErrorCodeNone)
    {
        NSLog(@"SENSOR FUSION ERROR %li", (long)status.errorCode);
        startTime = nil;
    }
}

- (void) startTimer
{
    startTime = [NSDate date];
}

- (void) startCalibration
{
    LOGME
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Hold the device steady and make sure the camera isn't blocked"];
    [self showProgressViewWithTitle:@"Calibrating"];
    
    isCalibrating = YES;
    
    [sensorFusion startSensorFusionWithDevice:[self.delegate getVideoDevice]];
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        LOGME
        isCalibrating = NO;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
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
