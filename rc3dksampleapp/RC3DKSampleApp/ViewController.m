//
//  ViewController.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "ViewController.h"
#import "LicenseHelper.h"
#import "RCSensorDelegate.h"

@implementation ViewController
{
    RCSensorFusion* sensorFusion;
    bool isStarted; // Keeps track of whether the start button has been pressed
    id<RCSensorDelegate> sensorDelegate;
}
@synthesize startStopButton, distanceText, statusLabel;

- (void)viewDidLoad
{
    [super viewDidLoad];

    sensorDelegate = [SensorDelegate sharedInstance];
    sensorFusion = [RCSensorFusion sharedInstance]; // The main class of the 3DK framework
    sensorFusion.delegate = self; // Tells RCSensorFusion where to send data to
    
    isStarted = false;
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];

    [self doSanityCheck];
}

- (void) doSanityCheck
{
    if ([RCDeviceInfo getDeviceType] == DeviceTypeUnknown)
    {
        statusLabel.text = @"Warning: This device is not supported by 3DK.";
        return;
    }
    
    #ifdef DEBUG
    statusLabel.text = @"Warning: You are running a debug build. The performance will be better with an optimized build.";
    #endif
}

- (void)startSensorFusion
{
    isStarted = true;
    [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    [sensorDelegate startAllSensors];
    [[RCSensorFusion sharedInstance] startSensorFusionWithDevice:[[RCAVSessionManager sharedInstance] videoDevice]];
}

- (void)stopSensorFusion
{
    [sensorFusion stopSensorFusion];
    [sensorDelegate stopAllSensors];
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    isStarted = false;
}

#pragma mark - RCSensorFusionDelegate methods

// Called after each video frame is processed ~ 30hz.
- (void)sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
    // Calculate and show the distance the device has moved from the start point
    float distanceFromStartPoint = sqrt(data.transformation.translation.x * data.transformation.translation.x + data.transformation.translation.y * data.transformation.translation.y + data.transformation.translation.z * data.transformation.translation.z);
    if (distanceFromStartPoint) distanceText.text = [NSString stringWithFormat:@"%0.3fm", distanceFromStartPoint];
}

// Called when sensor fusion status changes, including when errors occur.
- (void)sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if ([status.error isKindOfClass:[RCSensorFusionError class]])
    {
        [self handleSensorFusionError:(RCSensorFusionError*)status.error];
    }
    else if ([status.error isKindOfClass:[RCLicenseError class]])
    {
        [self handleLicenseProblem:(RCLicenseError*)status.error];
    }
    else if(status.runState == RCSensorFusionRunStateSteadyInitialization)
    {
        statusLabel.text = [NSString stringWithFormat:@"Initializing. Hold the device steady. %.0f%% complete.", status.progress * 100.];
    }
    else if(status.runState == RCSensorFusionRunStateRunning)
    {
        statusLabel.text = @"Move the device to measure the distance.";
    }
}

#pragma mark -

- (void) handleSensorFusionError:(RCSensorFusionError*)error
{
    switch (error.code)
    {
        case RCSensorFusionErrorCodeVision:
            statusLabel.text = @"Error: The camera cannot see well enough. Could be too dark, camera blocked, or featureless scene.";
            break;
        case RCSensorFusionErrorCodeTooFast:
            statusLabel.text = @"Error: The device was moved too fast. Try moving slower and smoother.";
            [self stopSensorFusion];
            break;
        case RCSensorFusionErrorCodeOther:
            statusLabel.text = @"Error: A fatal error has occured.";
            [self stopSensorFusion];
            break;
        default:
            statusLabel.text = @"Error: Unknown.";
            [self stopSensorFusion];
            break;
    }
}

- (void) handleLicenseProblem:(RCLicenseError*)error
{
    statusLabel.text = @"Error: License problem";
    [LicenseHelper showLicenseValidationError:error];
    [self stopSensorFusion];
}

// Event handler for the start/stop button
- (IBAction)buttonTapped:(id)sender
{
    if (isStarted)
    {
        [self stopSensorFusion];
    }
    else
    {
        [self startSensorFusion];
    }
}

//Resets the app if we are suspended
- (void) handlePause
{
    if(isStarted) [self stopSensorFusion];
}

@end
