//
//  ViewController.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "ViewController.h"
#import "LicenseHelper.h"
#import "RCSensorDelegate.h"
#import "RCDeviceInfo.h"
#import "RCVideoPreview.h"
#import "RCAVSessionManager.h"

@implementation ViewController
{
    RCSensorFusion* sensorFusion;
    bool isStarted; // Keeps track of whether the start button has been pressed
    id<RCSensorDelegate> sensorDelegate;
    RCVideoPreview *videoPreview;
}

@synthesize statusLabel;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    sensorDelegate = [SensorDelegate sharedInstance];
    sensorFusion = [RCSensorFusion sharedInstance]; // The main class of the 3DK framework
    sensorFusion.delegate = self; // Tells RCSensorFusion where to send data to
    
    isStarted = false;
    
    videoPreview = [[RCVideoPreview alloc] initWithFrame:self.view.frame];
    [[sensorDelegate getVideoProvider] setDelegate:videoPreview];
    [self.view addSubview:videoPreview];
    [self.view sendSubviewToBack:videoPreview];

    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    
    [self doSanityCheck];
    [self startSensorFusion];
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
    [sensorDelegate startAllSensors];
    [[sensorDelegate getVideoProvider] setDelegate:nil];
    [[RCSensorFusion sharedInstance] startSensorFusionWithDevice:[[RCAVSessionManager sharedInstance] videoDevice]];
}

- (void)stopSensorFusion
{
    [sensorFusion stopSensorFusion];
    [sensorDelegate stopAllSensors];
    [[sensorDelegate getVideoProvider] setDelegate:videoPreview];
    isStarted = false;
}

#pragma mark - RCSensorFusionDelegate methods

// Called after each video frame is processed ~ 30hz.
- (void)sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
    [videoPreview displaySensorFusionData:data];
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
