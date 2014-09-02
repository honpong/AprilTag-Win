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
    RCSensorFusionRunState currentRunState;
}
@synthesize messageLabel, bgImage;

+ (RCCalibration1 *)instantiateViewController
{
    UIStoryboard * calibrationStoryBoard;
    calibrationStoryBoard = [UIStoryboard storyboardWithName:@"Calibration" bundle:nil];
    return (RCCalibration1 *)[calibrationStoryBoard instantiateInitialViewController];
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
    currentRunState = RCSensorFusionRunStateInactive;
    
	isCalibrating = NO;
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.calibrationDelegate calibrationScreenDidAppear: @"Calibration1"];
    [super viewDidAppear:animated];
    [self startCalibration];
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

- (void) handleResume
{
    [self startCalibration];
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
        if(status.runState != currentRunState)
        {
            currentRunState = status.runState;
            switch(currentRunState)
            {
                case RCSensorFusionRunStateStaticCalibration:
                    [messageLabel setText:@"Place the device on a flat, stable surface, like a table."];
                    break;
                case RCSensorFusionRunStatePortraitCalibration:
                    [messageLabel setText:@"Hold the device steady in portrait orientation, perpendicular to the floor."];
                    bgImage.image = [UIImage imageNamed:@"holding_portrait.jpg"];
                    break;
                case RCSensorFusionRunStateLandscapeCalibration:
                    [messageLabel setText:@"Hold the device steady in landscape orientation, perpendicular to the floor."];
                    bgImage.image = [UIImage imageNamed:@"holding_landscape.jpg"];
                    break;
                case RCSensorFusionRunStateInactive:
                    [self calibrationFinished];
                    break;
                default: //should not happen
                    break;
            }
            [UIViewController attemptRotationToDeviceOrientation];
        }
        [self updateProgressView:status.progress];
    }
}

- (void) calibrationFinished
{
    [self stopCalibration];
    [self gotoNextScreen];
}

- (void) startCalibration
{
    isCalibrating = YES;
    
    [self showProgressViewWithTitle:@"Calibrating"];
    
    //This calibration step only requires motion data, no video
    [self.sensorDelegate startMotionSensors];
    sensorFusion.delegate = self;
    [sensorFusion startStaticCalibration];
}

- (void) stopCalibration
{
    isCalibrating = NO;
    
    [self.sensorDelegate stopAllSensors];
    [sensorFusion stopSensorFusion];
    sensorFusion.delegate = nil;
    
    [messageLabel setText:@"Place the device on a flat, stable surface, like a table."];
    [self hideProgressView];
    
}

- (void) gotoNextScreen
{
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationDidFinish)]) [self.calibrationDelegate calibrationDidFinish];
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
