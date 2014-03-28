//
//  MPCalibration3.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration3.h"
#import "MPCapturePhoto.h"

@interface RCCalibration3 ()

@end

@implementation RCCalibration3
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
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientation)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
    SENSOR_FUSION.delegate = self;
    [VIDEO_MANAGER setDelegate:videoPreview];
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.delegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.delegate calibrationScreenDidAppear: @"Calibration3"];
    [super viewDidAppear:animated];
    [self handleResume];
    [videoPreview setTransformFromCurrentVideoOrientationToOrientation:AVCaptureVideoOrientationLandscapeRight];
    [self handleOrientation];
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
}

- (void) handleResume
{
    // these should already be running, unless we paused. calling them if they're already running shouldn't be a problem.
    [SESSION_MANAGER startSession];
    [SENSOR_FUSION startProcessingVideoWithDevice:[SESSION_MANAGER videoDevice]];
    [VIDEO_MANAGER startVideoCapture];
}

- (IBAction) handleButton:(id)sender
{
    [self startCalibration];
}

- (void) gotoNextScreen
{
    [VIDEO_MANAGER setDelegate:nil];
    if ([self.delegate respondsToSelector:@selector(calibrationDidFinish)]) [self.delegate calibrationDidFinish];
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (isCalibrating && [SENSOR_FUSION isProcessingVideo])
    {
        if (!startTime)
            [self startTimer];

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
    
    if (data.sampleBuffer) [videoPreview displaySampleBuffer:data.sampleBuffer];
}

- (void) sensorFusionError:(NSError*)error
{
    DLog(@"SENSOR FUSION ERROR %i", error.code);
    startTime = nil;
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
    
    isCalibrating = YES;
    
    [VIDEO_MANAGER startVideoCapture];
    [SENSOR_FUSION startProcessingVideoWithDevice:[SESSION_MANAGER videoDevice]];
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        isCalibrating = NO;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
        [messageLabel setText:@"Hold the iPad steady in landscape orientation. Make sure the camera lens isn't blocked. Step 3 of 3."];
        [self hideProgress];
        [SENSOR_FUSION stopProcessingVideo];
        [VIDEO_MANAGER stopVideoCapture];
    }
}

- (void) finishCalibration
{
    [self stopCalibration];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED];
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
