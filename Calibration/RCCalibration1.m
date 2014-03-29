//
//  MPCalibrationVCViewController.m
//  MeasuredPhoto
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
}
@synthesize button, messageLabel;

+ (RCCalibration1*) instantiateViewControllerWithDelegate:(id)delegate
{
    // These three lines prevent the compiler from optimizing out the view controller classes
    // completely, since they are only presented in a storyboard which is not directly referenced anywhere.
    [RCCalibration1 class];
    [RCCalibration2 class];
    [RCCalibration3 class];
    
    UIStoryboard * calibrationStoryBoard;
    calibrationStoryBoard = [UIStoryboard storyboardWithName:@"Calibration" bundle:nil];
    RCCalibration1 * calibration1 = (RCCalibration1 *)[calibrationStoryBoard instantiateInitialViewController];
    calibration1.delegate = delegate;
    return calibration1;
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
	isCalibrating = NO;
}

- (void) viewDidAppear:(BOOL)animated
{
    if ([self.delegate respondsToSelector:@selector(calibrationScreenDidAppear:)])
        [self.delegate calibrationScreenDidAppear: @"Calibration1"];
    [super viewDidAppear:animated];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handlePause
{
    if (isCalibrating) [self resetCalibration];
}

- (IBAction) handleButton:(id)sender
{
    if (!isCalibrating) [self startCalibration];
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (isCalibrating)
    {
        if (data.status.calibrationProgress >= 1.)
        {
            [self calibrationFinished];
        }
        else
        {
            [self updateProgress:data.status.calibrationProgress];
        }
    }
}

- (void) sensorFusionError:(NSError*)error
{
    DLog(@"SENSOR FUSION ERROR %i", error.code);
}

- (void) calibrationFinished
{
    [self stopCalibration];
    
    RCCalibration2* cal2 = [self.storyboard instantiateViewControllerWithIdentifier:@"Calibration2"];
    cal2.delegate = self.delegate;
    cal2.videoDevice = self.videoDevice;
    cal2.videoProvider = self.videoProvider;
    [self presentViewController:cal2 animated:YES completion:nil];
}

- (void) startCalibration
{
    [self showProgressWithTitle:@"Calibrating"];
    SENSOR_FUSION.delegate = self;
    [SENSOR_FUSION startStaticCalibration];
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Don't touch the device until calibration has finished"];
    isCalibrating = YES;
}

- (void) stopCalibration
{
    [SENSOR_FUSION stopStaticCalibration];
    SENSOR_FUSION.delegate = nil;
    [self resetCalibration];
}

- (void) resetCalibration
{
    [button setTitle:@"Begin Calibration" forState:UIControlStateNormal];
    [messageLabel setText:@"Your device needs to be calibrated just once. Place it on a flat, stable surface, like a table."];
    isCalibrating = NO;
    [self hideProgress];
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
