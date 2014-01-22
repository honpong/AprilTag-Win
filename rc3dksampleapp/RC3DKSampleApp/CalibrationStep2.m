//
//  CalibrationStep2.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 8/29/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "CalibrationStep2.h"
#import "CalibrationStep3.h"
#import "MBProgressHUD.h"
#import "VideoManager.h"
#import "LicenseHelper.h"

@implementation CalibrationStep2
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
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientation)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
    [RCSensorFusion sharedInstance].delegate = self;
    [[VideoManager sharedInstance] setupWithSession:[AVSessionManager sharedInstance].session];
}

- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [[AVSessionManager sharedInstance] startSession];
    [self handleOrientation];
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (void) handleOrientation
{
    // must be done on UI thread
    UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
    if (orientation == UIDeviceOrientationPortrait)
    {
        button.enabled = YES;
        [button setTitle:@"Tap here to begin calibration" forState:UIControlStateNormal];
    }
    else
    {
        button.enabled = NO;
        [button setTitle:@"Hold in portrait orientation" forState:UIControlStateNormal];
    }
}

- (void) handlePause
{
    [self stopCalibration];
}

- (void) handleResume
{
    // these should already be running, unless we paused. calling them if they're already running shouldn't be a problem.
    [[AVSessionManager sharedInstance] startSession];
}

- (IBAction) handleButton:(id)sender
{
    [self startCalibration];
}

- (void) gotoNextScreen
{
    CalibrationStep3* cal3 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration3"];
    cal3.delegate = self.delegate;
    [self presentViewController:cal3 animated:YES completion:nil];
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (isCalibrating && [[RCSensorFusion sharedInstance] isProcessingVideo])
    {
        if(!startTime)
            [self startTimer];

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

- (void) sensorFusionError:(NSError*)error
{
    NSLog(@"SENSOR FUSION ERROR %i", error.code);
    [self startTimer];
}

- (void) startTimer
{
    startTime = [NSDate date];
}

- (void) startCalibration
{
    [RCSensorFusion sharedInstance].delegate = self;
    [[RCSensorFusion sharedInstance] validateLicense:API_KEY withCompletionBlock:^(int licenseType, int licenseStatus){
        if(licenseStatus == RCLicenseStatusOK)
        {
            [button setTitle:@"Calibrating" forState:UIControlStateNormal];
            [messageLabel setText:@"Hold the device steady"];
            [self showProgressWithTitle:@"Calibrating"];
            [[RCSensorFusion sharedInstance] startProcessingVideoWithDevice:[[AVSessionManager sharedInstance] videoDevice]];
            [[VideoManager sharedInstance] startVideoCapture];
            isCalibrating = YES;
        }
        else
        {
            [LicenseHelper showLicenseStatusError:licenseStatus];
        }
    } withErrorBlock:^(NSError * error) {
        [LicenseHelper showLicenseValidationError:error];
    }];
}

- (void) stopCalibration
{
    if (isCalibrating)
    {
        isCalibrating = NO;
        [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
        [messageLabel setText:@"Hold the iPad steady in portrait orientation. Step 2 of 3."];
        [self hideProgress];
        [[VideoManager sharedInstance] stopVideoCapture];
        [[RCSensorFusion sharedInstance] stopProcessingVideo];
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
