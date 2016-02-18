//
//  ViewController.m
//  QRCodeSample
//
//  Created by Ben Hirashima on 5/8/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import "ViewController.h"

#ifndef SDK_LICENSE_KEY
#error You must insert your 3DK license key here and delete this line
#define SDK_LICENSE_KEY @"YOUR_API_KEY_HERE"
#endif

@interface ViewController ()

@end

@implementation ViewController
{
    RCSensorFusion* sensorFusion;
    RCSensorManager* sensorManager;
    RCSensorFusionRunState currentRunState;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorManager = [RCSensorManager sharedInstance];
    [sensorFusion setLicenseKey:SDK_LICENSE_KEY];
    currentRunState = RCSensorFusionRunStateInactive;
    
    self.statusLabel.text = @"Press the Start button to begin searching for QR codes.";
    self.distanceLabel.text = nil;
}

- (IBAction)handleButtonTap:(id)sender
{
    if (currentRunState == RCSensorFusionRunStateInactive)
        [self startSensorFusion];
    else
        [self stopSensorFusion];
}

- (void) startSensorFusion
{
    self.statusLabel.text = @"Hold still...";
    [self.button setTitle:@"Stop" forState:UIControlStateNormal];
    
    [sensorManager startAllSensors]; // sensors must be running before sensor fusion is started
    [sensorFusion setDelegate:self];
    [sensorFusion startSensorFusionWithDevice: sensorManager.getVideoDevice];
    [sensorFusion startQRDetectionWithData:nil // pass nil to search for any QR code. pass a string to search for a QR code with that payload.
                             withDimension:0.142081 // the physical size of the QR code, in meters
                          withAlignGravity:YES]; // z axis will be aligned with gravity
}

- (void) stopSensorFusion
{
    [sensorFusion stopSensorFusion];
    [sensorFusion setDelegate:nil];
    [sensorManager stopAllSensors];
    currentRunState = RCSensorFusionRunStateInactive;
    
    self.statusLabel.text = @"Sensor fusion stopped.";
    self.distanceLabel.text = nil;
    [self.button setTitle:@"Start" forState:UIControlStateNormal];
}

- (void)sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    currentRunState = status.runState;
    
    if (status.error && status.error.code > 1) // stop on error codes greater than 1. 1 is a vision error that is usually recoverable.
    {
        [self stopSensorFusion];
        self.statusLabel.text = [NSString stringWithFormat:@"ERROR code: %li", (long)status.error.code];
        return;
    }
    
    switch (status.runState)
    {
        case RCSensorFusionRunStateRunning:
            self.statusLabel.text = @"Looking for QR codes...";
            break;
            
        default:
            self.statusLabel.text = @"";
            break;
    }
}

- (void)sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
    if (data.originQRCode)
    {
        float distanceFromQRCode = sqrt(data.transformation.translation.x * data.transformation.translation.x + data.transformation.translation.y * data.transformation.translation.y + data.transformation.translation.z * data.transformation.translation.z);
        
        self.statusLabel.text = [NSString stringWithFormat:@"QR code found: %@", data.originQRCode];
        self.distanceLabel.text = [NSString stringWithFormat:@"Distance: %0.3fm", distanceFromQRCode];
    }
}

@end
