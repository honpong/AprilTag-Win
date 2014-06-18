//
//  RCCalibration1.m
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration1.h"
#import "RCCalibration2.h"
#import "RCCalibration3.h"

@implementation RCCalibration1
{
    BOOL isCalibrating;
    MBProgressHUD *progressView;
    RCSensorFusion* sensorFusion;
}
@synthesize button, messageLabel;

+ (RCCalibration1 *)instantiateViewController
{
    UIStoryboard * calibrationStoryBoard;
    calibrationStoryBoard = [UIStoryboard storyboardWithName:@"Calibration" bundle:nil];
    return (RCCalibration1 *)[calibrationStoryBoard instantiateInitialViewController];
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
    
	isCalibrating = NO;
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.calibrationDelegate calibrationScreenDidAppear: @"Calibration1"];
    [super viewDidAppear:animated];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handlePause
{
    if(isCalibrating) [self stopCalibration];
}

- (IBAction) handleButton:(id)sender
{
    if (!isCalibrating) [self startCalibration];
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
}

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if ([status.error isKindOfClass:[RCLicenseError class]])
    {
        NSString * message = [NSString stringWithFormat:@"There was a problem validating your license. The license error code is: %li.", (long)status.error.code];
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"3DK License Error"
                                                        message:message
                                                       delegate:self
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        [alert show];
        [self stopCalibration];
    }
    else if (isCalibrating)
    {
        if (status.runState == RCSensorFusionRunStateInactive)
        {
            [self calibrationFinished];
        }
        else
        {
            [self updateProgressView:status.progress];
        }
    }
}

- (void) calibrationFinished
{
    [self stopCalibration];
    
    RCCalibration2* cal2 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration2"];
    cal2.calibrationDelegate = self.calibrationDelegate; // pass the RCCalibrationDelegate object on to the next view controller
    cal2.sensorDelegate = self.sensorDelegate; // pass the RCSensorDelegate object on to the next view controller
    [self presentViewController:cal2 animated:YES completion:nil];
}

- (void) startCalibration
{
    isCalibrating = YES;
    
    [self showProgressViewWithTitle:@"Calibrating"];
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Don't touch the device until calibration has finished"];
    
    //This calibration step only requires motion data, no video
    [self.sensorDelegate startMotionSensors];
    sensorFusion.delegate = self;
    [sensorFusion startStaticCalibration];
}

- (void) stopCalibration
{
    [self.sensorDelegate stopAllSensors];
    [sensorFusion stopSensorFusion];
    sensorFusion.delegate = nil;
    
    [button setTitle:@"Tap here to begin calibration" forState:UIControlStateNormal];
    [messageLabel setText:@"Your device needs to be calibrated just once. Place it on a flat, stable surface, like a table."];
    [self hideProgressView];
    
    isCalibrating = NO;
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
