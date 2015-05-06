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

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self.statusLabel setText:@"Place the device face up on a table."];
    
    [self startCalibration];
}

- (void) startCalibration
{
    [[RCSensorManager sharedInstance] startMotionSensors];
    [[RCSensorFusion sharedInstance] setDelegate:self];
    [[RCSensorFusion sharedInstance] startStaticCalibration];
}

- (void) stopCalibration
{
    [[RCSensorManager sharedInstance] stopAllSensors];
    [[RCSensorFusion sharedInstance] stopSensorFusion];
    [[RCSensorFusion sharedInstance] setDelegate:nil];
}

- (void) updateProgressDisplay:(float) progress
{
    NSString* progressString = [NSString stringWithFormat:@"Hold still... 00.0%f%%", progress * 100.];
    [self.statusLabel setText:progressString];
}

#pragma mark - RCSensorFusionDelegate methods

- (void)sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if (status.runState == RCSensorFusionRunStateStaticCalibration)
    {
        if (status.progress < 1.)
        {
            [self updateProgressDisplay:status.progress];
        }
        else
        {
            [self.statusLabel setText:@"Hold device in portrait orientation"];
        }
    }
    else if (status.runState == RCSensorFusionRunStatePortraitCalibration)
    {
        if (status.progress < 1.)
        {
            [self updateProgressDisplay:status.progress];
        }
        else
        {
            [self.statusLabel setText:@"Hold device in landscape orientation"];
        }
    }
    else if (status.runState == RCSensorFusionRunStateLandscapeCalibration)
    {
        if (status.progress < 1.)
        {
            [self updateProgressDisplay:status.progress];
        }
        else
        {
            [self.statusLabel setText:@"Calibration complete"];
            [self stopCalibration];
        }
    }
}

- (void)sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
    
}


@end
