//
//  RCCalibration3.m
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration3.h"
#import "MBProgressHUD.h"

@implementation RCCalibration3
{
    MBProgressHUD *progressView;
    RCSensorFusion* sensorFusion;
    float currentProgress;
}
@synthesize messageLabel;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (!self) return nil;
    
    sensorFusion = [RCSensorFusion sharedInstance];
    
    return self;
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (void) viewDidAppear:(BOOL)animated
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    
    currentProgress = 0.;
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.calibrationDelegate calibrationScreenDidAppear: self];
    [super viewDidAppear:animated];
    [self createProgressViewWithTitle:@"Calibrating"];
    sensorFusion.delegate = self;
}

- (void) viewWillDisappear:(BOOL)animated
{
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIApplicationWillResignActiveNotification
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIApplicationDidBecomeActiveNotification
                                                  object:nil];
    
    [super viewWillDisappear:animated];
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

- (void) handlePause
{
    [self stopCalibration];
}

- (void) handleResume
{
    [self.presentingViewController.presentingViewController dismissViewControllerAnimated:NO completion:nil];
}

- (void) gotoNextScreen
{
    messageLabel.text = @"Please wait...";
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIApplicationWillResignActiveNotification
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIApplicationDidBecomeActiveNotification
                                                  object:nil];
    
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationDidFinish:)]) [self.calibrationDelegate calibrationDidFinish:self];
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
        if(status.runState == RCSensorFusionRunStateInactive)
        {
            [self hideProgressView];
            [self stopCalibration];
            [self gotoNextScreen];
        }
        if(status.progress != currentProgress)
        {
            if(status.progress > 0. && currentProgress <= 0.)
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
    sensorFusion.delegate = nil;
    [self hideProgressView];
    [self.calibrationDelegate stopMotionSensors];
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
