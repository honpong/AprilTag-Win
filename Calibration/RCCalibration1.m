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
    float currentProgress;
}
@synthesize messageLabel;

+ (RCCalibration1 *)instantiateViewController
{
    UIStoryboard * calibrationStoryBoard;
    calibrationStoryBoard = [UIStoryboard storyboardWithName:@"Calibration" bundle:nil];
    return (RCCalibration1 *)[calibrationStoryBoard instantiateInitialViewController];
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (NSUInteger) supportedInterfaceOrientations { return UIInterfaceOrientationMaskPortrait; }

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
    currentProgress = 0.;
    
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
        if(status.runState == RCSensorFusionRunStatePortraitCalibration)
        {
            [self hideProgressView];
            [self gotoNextScreen];
        }
        if(status.progress != currentProgress)
        {
            if(status.progress >= 0.02 && currentProgress < 0.02) //delay showing it until we've made a bit of progress so it doesn't flash on and reset as soon as we get close
            {
                [self showProgressView];
            }
            [self updateProgressView:status.progress];
            currentProgress = status.progress;
        }
    }
}

- (void) startCalibration
{
    isCalibrating = YES;
    [self createProgressViewWithTitle:@"Calibrating"];
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
    RCCalibration2* cal2 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration2"];
    cal2.calibrationDelegate = self.calibrationDelegate; // pass the RCCalibrationDelegate object on to the next view controller
    cal2.sensorDelegate = self.sensorDelegate; // pass the RCSensorDelegate object on to the next view controller
    sensorFusion.delegate = cal2;
    [self presentViewController:cal2 animated:YES completion:nil];
}

- (void)createProgressViewWithTitle:(NSString*)title
{
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.view addSubview:progressView];
    progressView.labelText = title;
    [progressView hide:YES];
}

- (void)showProgressView
{
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
