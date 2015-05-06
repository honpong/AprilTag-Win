//
//  ViewController.m
//  CalibrationSample
//
//  Created by Ben Hirashima on 5/6/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import "ViewController.h"

@interface ViewController ()

@end

@implementation ViewController

#ifndef SDK_LICENSE_KEY
#error You must insert your 3DK license key here and delete this line
#define SDK_LICENSE_KEY @"YOUR_API_KEY_HERE"
#endif

- (void)viewDidLoad
{
    [super viewDidLoad];
    [[RCSensorFusion sharedInstance] setLicenseKey:SDK_LICENSE_KEY];
}

- (void)viewDidAppear:(BOOL)animated
{
    [self startCalibration];
}

- (void) startCalibration
{
    [self.statusLabel setText:@"Place the device face up on a table."];
    
    [[RCSensorManager sharedInstance] startMotionSensors]; // motion sensors must be running before calibration is started
    [[RCSensorFusion sharedInstance] setDelegate:self]; // this causes sensorFusionDidChangeStatus to be called when the status changes
    [[RCSensorFusion sharedInstance] startStaticCalibration]; // starts calibration
}

- (void) stopCalibration
{
    [[RCSensorFusion sharedInstance] stopSensorFusion]; // stops calibration. necessary to ensure that the calibration is saved!
    [[RCSensorFusion sharedInstance] setDelegate:nil]; // we no longer need to be a delegate
    [[RCSensorManager sharedInstance] stopAllSensors]; // shuts down motion sensors
    
    [self.statusLabel setText:@"Calibration complete"];
}

- (void) updateProgressDisplay:(float) progress
{
    NSString* progressString = [NSString stringWithFormat:@"Hold still... %.f%%", progress * 100.];
    [self.statusLabel setText:progressString];
}

#pragma mark - RCSensorFusionDelegate methods

- (void)sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if (status.runState == RCSensorFusionRunStateStaticCalibration) // static calibration (face up) is the first of three calibration stages
    {
        if (status.progress > 0)
        {
            [self updateProgressDisplay:status.progress];
        }
    }
    else if (status.runState == RCSensorFusionRunStatePortraitCalibration) // once progress during the static calibration stage reaches 100%, it automatically switches to the next stage, which is portrait calibration
    {
        if (status.progress == 0)
        {
            [self.statusLabel setText:@"Hold device in portrait orientation"];
        }
        else if (status.progress > 0 && status.progress < 1.)
        {
            [self updateProgressDisplay:status.progress];
        }
    }
    else if (status.runState == RCSensorFusionRunStateLandscapeCalibration) // once progress during the portrait calibration stage reaches 100%, it automatically switches to the next stage, which is landscape calibration
    {
        if (status.progress == 0)
        {
            [self.statusLabel setText:@"Hold device in landscape orientation"];
        }
        else if (status.progress > 0 && status.progress < 1.)
        {
            [self updateProgressDisplay:status.progress];
        }
    }
    else if (status.runState == RCSensorFusionRunStateInactive) // when the runState returns to inactive, we know the calibration is finished
    {
        [self stopCalibration];
    }
}

- (void)sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
    
}


@end
