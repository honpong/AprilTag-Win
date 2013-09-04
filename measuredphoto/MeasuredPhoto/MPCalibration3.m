//
//  MPCalibration3.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPCalibration3.h"
#import "MPMeasuredPhotoVC.h"

@interface MPCalibration3 ()

@end

@implementation MPCalibration3
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
    SENSOR_FUSION.delegate = self;
}

- (void) viewDidAppear:(BOOL)animated
{
    [self handleOrientation:self.interfaceOrientation];
}

- (void) willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    [self handleOrientation:toInterfaceOrientation];
}

- (void) handleOrientation:(UIInterfaceOrientation)orientation
{
    if (orientation == UIInterfaceOrientationLandscapeRight || orientation == UIInterfaceOrientationLandscapeLeft)
    {
        button.enabled = YES;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
    }
    else
    {
        button.enabled = NO;
        [button setTitle:@"Rotate to landscape" forState:UIControlStateNormal];
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
    [SENSOR_FUSION startProcessingVideo];
    [VIDEO_MANAGER startVideoCapture];
}

- (IBAction) handleButton:(id)sender
{
    [self startCalibration];
}

- (void) gotoNextScreen
{
    MPMeasuredPhotoVC* mp = [self.storyboard instantiateViewControllerWithIdentifier:@"MeasuredPhoto"];
    self.view.window.rootViewController = mp;
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

- (void) sensorFusionError:(RCSensorFusionError*)error
{
    DLog(@"ERROR %@", error.debugDescription);
    [SENSOR_FUSION resetSensorFusion];
    [SENSOR_FUSION startProcessingVideo];
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
    
    isCalibrating = YES;
    
    [VIDEO_MANAGER startVideoCapture];
    [SENSOR_FUSION startProcessingVideo];
    
    [self startTimer];
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        isCalibrating = NO;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
        [messageLabel setText:@"Hold the iPad steady in landscape orientation. Step 3 of 3."];
        [self hideProgress];
        [SENSOR_FUSION stopProcessingVideo];
        [VIDEO_MANAGER stopVideoCapture];
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
