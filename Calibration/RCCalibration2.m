//
//  MPCalibration2.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration2.h"
#import "RCCalibration3.h"
#import "MBProgressHUD.h"

@implementation RCCalibration2
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
    
    [RCVideoPreview class]; // keeps this class from being optimized out by the complier, since it isn't referenced anywhere besides in the storyboard
    [self.delegate getVideoProvider].delegate = self.videoPreview;
    
    [self handleResume];
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.delegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.delegate calibrationScreenDidAppear: @"Calibration2"];
    [super viewDidAppear:animated];
    [videoPreview setVideoOrientation:AVCaptureVideoOrientationPortrait];
    [self handleOrientation];
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (void) handleOrientation
{
    // must be done on UI thread
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    if (orientation == UIDeviceOrientationPortrait)
    {
        button.enabled = YES;
        [button setTitle:@"Tap here to begin calibration" forState:UIControlStateNormal];
    }
    else
    {
        button.enabled = NO;
        [button setTitle:@"Hold in portrait orientation" forState:UIControlStateNormal];
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
    RCCalibration3* cal3 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration3"];
    cal3.delegate = self.delegate;
    [self presentViewController:cal3 animated:YES completion:nil];
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

- (void) sensorFusionError:(NSError*)error
{
    NSLog(@"SENSOR FUSION ERROR %li", (long)error.code);
    startTime = nil;
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
    
    sensorFusion.delegate = self;
    [sensorFusion startProcessingVideoWithDevice:[self.delegate getVideoDevice]];

    isCalibrating = YES;
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        LOGME
        isCalibrating = NO;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
        [messageLabel setText:@"Hold the iPad steady in portrait orientation. Make sure the camera lens isn't blocked. Step 2 of 3."];
        [self hideProgressView];
        startTime = nil;
        [sensorFusion stopProcessingVideo];
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
