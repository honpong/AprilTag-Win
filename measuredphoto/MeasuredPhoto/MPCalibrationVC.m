//
//  MPCalibrationVCViewController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPCalibrationVC.h"

@implementation MPCalibrationVC
{
    BOOL isCalibrating;
    MBProgressHUD *progressView;
}
@synthesize button, messageLabel, delegate;

- (void)viewDidLoad
{
    [super viewDidLoad];
	isCalibrating = NO;
    SENSOR_FUSION.delegate = self;
}

- (IBAction)handleButton:(id)sender
{
    if (!isCalibrating) [self startCalibration];
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (isCalibrating)
    {
        if (data.status.calibrationProgress >= 1.)
        {
            [self stopCalibration];
        }
        else
        {
            [self updateProgress:data.status.calibrationProgress];
        }
    }
}

- (void) sensorFusionError:(RCSensorFusionError*)error
{
    DLog(@"ERROR %@", error.debugDescription);
}

- (void) startCalibration
{
    [self showProgressWithTitle:@"Calibrating"];
    [SENSOR_FUSION startStaticCalibration];
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Don't touch the device until calibration has finished"];
    isCalibrating = YES;
}

- (void) stopCalibration
{
    [SENSOR_FUSION stopStaticCalibration];
//    [button setTitle:@"Start Calibration" forState:UIControlStateNormal];
//    [messageLabel setText:@"Calibration is complete"];
    isCalibrating = NO;
    [self hideProgress];
    [delegate calibrationDidComplete];    
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
