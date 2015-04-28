//
//  ViewController.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "ViewController.h"
#import "LicenseHelper.h"
#import "ARRenderer.h"

//#define LOOK_FOR_QR_CODES

@implementation ViewController
{
    RCSensorFusion* sensorFusion;
    bool isStarted; // Keeps track of whether the start button has been pressed
    RCSensorManager* sensorManager;
    RCVideoPreview *videoPreview;
    ARRenderer *arDelegate;
    RCSensorFusionRunState currentRunState;
}

@synthesize statusLabel, progressBar;

- (void)viewDidLoad
{
    [super viewDidLoad];
    [progressBar setHidden:true];
    
    sensorManager = [RCSensorManager sharedInstance];
    sensorFusion = [RCSensorFusion sharedInstance]; // The main class of the 3DK framework
    sensorFusion.delegate = self; // Tells RCSensorFusion where to send data to
    
    isStarted = false;
    
    videoPreview = [[RCVideoPreview alloc] initWithFrame:self.view.frame];
    [[sensorManager getVideoProvider] addDelegate:videoPreview];
    
    arDelegate = [[ARRenderer alloc] init];

    [self.view addSubview:videoPreview];
    [self.view sendSubviewToBack:videoPreview];
    [videoPreview setAutoresizingMask:UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth];
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

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (void) handleResume
{
    [sensorManager startAllSensors];
    [self setInitialText];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    videoPreview.orientation = toInterfaceOrientation;
}

- (void) setInitialText
{
#ifdef DEBUG
    statusLabel.text = @"Warning: You are running a debug build. The performance will be better with an optimized build.\nTap to initialize.";
#else
    if ([RCDeviceInfo getDeviceType] == DeviceTypeUnknown)
    {
        statusLabel.text = @"Warning: This device is not supported by 3DK.\nTap to initialize.";
        return;
    }
    else
    {
        statusLabel.text = @"Tap to initialize.";
    }
#endif
}

- (IBAction)handleSingleTap:(UITapGestureRecognizer *)sender
{
    if(isStarted)
    {
        [self stopSensorFusion];
        [self setInitialText];
    }
    else
    {
        [self startSensorFusion];
    }
}

- (void)startSensorFusion
{
    isStarted = true;
    [[sensorManager getVideoProvider] removeDelegate:videoPreview];
    [[RCSensorFusion sharedInstance] startSensorFusionWithDevice:[[RCAVSessionManager sharedInstance] videoDevice]];
    [progressBar setHidden:false];
#ifdef LOOK_FOR_QR_CODES
    [[RCSensorFusion sharedInstance] startQRDetectionWithData:nil withDimension:0.1825 withAlignGravity:true];
#endif
    statusLabel.text = @"Initializing. Hold the device steady.";
}

- (void)stopSensorFusion
{
    videoPreview.delegate = nil;
    [progressBar setHidden:true];
    [sensorFusion stopSensorFusion];
    [[sensorManager getVideoProvider] addDelegate:videoPreview];
#ifdef LOOK_FOR_QR_CODES
    [[RCSensorFusion sharedInstance] stopQRDetection];
#endif
    isStarted = false;
}

#pragma mark - RCSensorFusionDelegate methods

// Called after each video frame is processed ~ 30hz.
- (void)sensorFusionDidUpdateData:(RCSensorFusionData *)data
{
    //as long as we are initializing, update the initial camera pose
    if(currentRunState == RCSensorFusionRunStateSteadyInitialization) arDelegate.initialCamera = data.cameraTransformation;

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
        progressBar.progress = status.progress;
    }
    else if(status.runState == RCSensorFusionRunStateRunning && currentRunState != RCSensorFusionRunStateRunning)
    {
        videoPreview.delegate = arDelegate;
        [progressBar setHidden:true];
        statusLabel.text = @"AR view active. Move to view. Tap to stop.";
    }
    currentRunState = status.runState;
}

#pragma mark -

- (void) handleSensorFusionError:(RCSensorFusionError*)error
{
    switch (error.code)
    {
        case RCSensorFusionErrorCodeVision:
            statusLabel.text = @"Alert: The camera cannot see well enough. Could be too dark, camera blocked, or featureless scene.";
            break;
        case RCSensorFusionErrorCodeTooFast:
            statusLabel.text = @"Error: The device was moved too fast. Try moving slower and smoother.\nTap to re-initialize.";
            [self stopSensorFusion];
            break;
        case RCSensorFusionErrorCodeOther:
            statusLabel.text = @"Error: A fatal error has occured.\nTap to re-initialize.";
            [self stopSensorFusion];
            break;
        default:
            statusLabel.text = @"Error: Unknown.\nTap to re-initialize.";
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
        [self setInitialText];
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
    [sensorManager stopAllSensors];
}

@end
