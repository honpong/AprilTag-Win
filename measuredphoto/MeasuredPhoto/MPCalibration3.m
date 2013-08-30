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
}

- (void) handlePause
{
    if (isCalibrating)
    {
        [SENSOR_FUSION stopProcessingVideo];
        [VIDEO_MANAGER stopVideoCapture];
        isCalibrating = NO;
    }
    [SESSION_MANAGER endSession];
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
    
}

- (void) sensorFusionError:(RCSensorFusionError*)error
{
    DLog(@"ERROR %@", error.debugDescription);
}

- (void)timerTick:(NSTimer*)theTimer
{
    NSDate* dateStarted = [theTimer.userInfo objectForKey:KEY_DATE_STARTED];
    float progress = -[dateStarted timeIntervalSinceNow] / 5.;
    
    if (progress < 1.)
    {
        [self updateProgress:progress];
    }
    else
    {
        [theTimer invalidate];
        [self finishCalibration];
    }
}

- (void) startCalibration
{
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Hold the device steady"];
    [self showProgressWithTitle:@"Calibrating"];
    
    isCalibrating = YES;
    
    [NSTimer scheduledTimerWithTimeInterval:0.1
                                     target:self
                                   selector:@selector(timerTick:)
                                   userInfo:[NSDictionary dictionaryWithObject:[NSDate date] forKey:KEY_DATE_STARTED]
                                    repeats:YES];
}

- (void) finishCalibration
{
    [SENSOR_FUSION stopProcessingVideo];
    [VIDEO_MANAGER stopVideoCapture];
    
    [self hideProgress];
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
