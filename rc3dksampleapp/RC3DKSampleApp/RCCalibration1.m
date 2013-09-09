//
//  RCCalibrationVCViewController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCalibration1.h"
#import "RCCalibration2.h"
#import "RCCalibration3.h"
#import "MBProgressHUD.h"

@implementation RCCalibration1
{
    BOOL isCalibrating;
    MBProgressHUD *progressView;
}
@synthesize button, messageLabel;

+ (UIViewController *) instantiateViewControllerWithDelegate:(id)delegate
{
    // These three lines prevent the compiler from optimizing out the view controller classes
    // completely, since they are only presented in a storyboard which is not directly referenced anywhere.
    [RCCalibration1 class];
    [RCCalibration2 class];
    [RCCalibration3 class];

    UIStoryboard * calibrationStoryBoard;
    if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
        calibrationStoryBoard = [UIStoryboard storyboardWithName:@"RCCalibration_iPad" bundle:nil];
    else
        calibrationStoryBoard = [UIStoryboard storyboardWithName:@"RCCalibration_iPhone" bundle:nil];
    RCCalibration1 * rc = (RCCalibration1 *)[calibrationStoryBoard instantiateInitialViewController];
    rc.delegate = delegate;
    return rc;
}

- (void)viewDidLoad
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

- (IBAction)handleButton:(id)sender
{
    if (!isCalibrating) [self startCalibration];
//    [self calibrationFinished];
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
    NSLog(@"SENSOR FUSION ERROR %i", error.code);
}

- (void) calibrationFinished
{
    [self stopCalibration];
    
    RCCalibration2 * cal2 = (RCCalibration2 *)[self.storyboard instantiateViewControllerWithIdentifier:@"Calibration2"];
    cal2.delegate = self.delegate;
    [self presentViewController:cal2 animated:YES completion:nil];
}

- (void) startCalibration
{
    [self showProgressWithTitle:@"Calibrating"];
    [RCSensorFusion sharedInstance].delegate = self;
    // Initialize sensor fusion.
    [[RCSensorFusion sharedInstance] startInertialOnlyFusion];
    [[RCSensorFusion sharedInstance] startStaticCalibration];
    [button setTitle:@"Calibrating" forState:UIControlStateNormal];
    [messageLabel setText:@"Don't touch the device until calibration has finished"];
    isCalibrating = YES;
}

- (void) stopCalibration
{
    [[RCSensorFusion sharedInstance] stopStaticCalibration];
    [RCSensorFusion sharedInstance].delegate = nil;
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
