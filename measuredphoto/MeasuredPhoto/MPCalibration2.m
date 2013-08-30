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
    
    SENSOR_FUSION.delegate = self;
    [SENSOR_FUSION startProcessingVideo];
    [VIDEO_MANAGER startVideoCapture];
        
    [NSTimer scheduledTimerWithTimeInterval:0.1
                                     target:self
                                   selector:@selector(timerTick:)
                                   userInfo:[NSDictionary dictionaryWithObject:[NSDate date] forKey:KEY_DATE_STARTED]
                                    repeats:YES];
}

- (void) finishCalibration
{
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
