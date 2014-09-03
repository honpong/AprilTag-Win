//
//  RCCalibration2.m
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration2.h"
#import "RCCalibration3.h"
#import "MBProgressHUD.h"

@implementation RCCalibration2
{
    MBProgressHUD *progressView;
    RCSensorFusion* sensorFusion;
    float currentProgress;
}
@synthesize messageLabel;

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
}

- (void) viewDidAppear:(BOOL)animated
{
    currentProgress = 0.;
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.calibrationDelegate calibrationScreenDidAppear: @"Calibration2"];
    [super viewDidAppear:animated];
    [self createProgressViewWithTitle:@"Calibrating"];
    sensorFusion.delegate = self;
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (void) handlePause
{
    [self stopCalibration];
}

- (void) handleResume
{
    //TODO: go to calibration screen 1
    [self.presentingViewController dismissViewControllerAnimated:NO completion:nil];
}

- (void) gotoNextScreen
{
    RCCalibration3* cal3 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration3"];
    cal3.calibrationDelegate = self.calibrationDelegate;
    cal3.sensorDelegate = self.sensorDelegate;
    sensorFusion.delegate = cal3;
    [self presentViewController:cal3 animated:YES completion:nil];
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
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
    else
    {
        if(status.runState == RCSensorFusionRunStateLandscapeCalibration)
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

- (void) stopCalibration
{
    LOGME
    [self hideProgressView];
    [self.sensorDelegate stopAllSensors];
    [sensorFusion stopSensorFusion];
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
