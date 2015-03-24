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

+ (RCCalibration1 *)instantiateFromQuickstartKit
{
    NSBundle* quickstartBundle = [NSBundle bundleWithURL:[[NSBundle mainBundle] URLForResource:@"QuickstartResources" withExtension:@"bundle"]];
    UIStoryboard* calStoryboard = [UIStoryboard storyboardWithName:@"Calibration" bundle:quickstartBundle];
    RCCalibration1* calibration1 = [calStoryboard instantiateInitialViewController];
    return calibration1;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (!self) return nil;
    
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
    currentRunState = RCSensorFusionRunStateInactive;
    currentProgress = 0.;
    
    isCalibrating = NO;
    
    return self;
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (NSUInteger) supportedInterfaceOrientations { return UIInterfaceOrientationMaskPortrait; }

- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    [self.messageLabel setText:@"We need to calibrate your sensors just once. Place the device on a flat, stable surface, like a table."];
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self;
    currentRunState = RCSensorFusionRunStateInactive;
    currentProgress = 0.;
    
    isCalibrating = NO;
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.calibrationDelegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.calibrationDelegate calibrationScreenDidAppear: self];
    [super viewDidAppear:animated];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    [self handleResume];
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
            if(status.progress > 0. && currentProgress <= 0.)
            {
                [messageLabel setText:@"Don't touch the device or the table."];
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
    [self.calibrationDelegate startMotionSensors];
    sensorFusion.delegate = self;
    [sensorFusion startStaticCalibration];
}

- (void) stopCalibration
{
    isCalibrating = NO;
    
    [self.calibrationDelegate stopMotionSensors];
    [sensorFusion stopSensorFusion];
    sensorFusion.delegate = nil;
    
    [self hideProgressView];
}

- (void) gotoNextScreen
{
    RCCalibration2* cal2 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration2"];
    cal2.calibrationDelegate = self.calibrationDelegate; // pass the RCCalibrationDelegate object on to the next view controller
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
