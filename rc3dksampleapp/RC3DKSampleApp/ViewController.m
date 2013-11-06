//
//  ViewController.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "ViewController.h"
#import "ConnectionManager.h"
#import "LicenseHelper.h"

@implementation ViewController
{
    AVSessionManager* avSessionManager;
    MotionManager* motionManager;
    LocationManager* locationManager;
    VideoManager* videoManager;
    ConnectionManager * connectionManager;
    RCSensorFusion* sensorFusion;
    bool isStarted; // Keeps track of whether the start button has been pressed
}
@synthesize startStopButton, distanceText, statusLabel;

- (void)viewDidLoad
{
    [super viewDidLoad];

    avSessionManager = [AVSessionManager sharedInstance]; // Manages the AV session
    videoManager = [VideoManager sharedInstance]; // Manages video capture
    motionManager = [MotionManager sharedInstance]; // Manages motion capture
    locationManager = [LocationManager sharedInstance]; // Manages location aquisition
    sensorFusion = [RCSensorFusion sharedInstance]; // The main class of the 3DK framework
    sensorFusion.delegate = self; // Tells RCSensorFusion where to send data to
    
    [videoManager setupWithSession:avSessionManager.session]; // The video manager must be initialized with an AVCaptureSession object

    [motionManager startMotionCapture]; // Starts sending accelerometer and gyro updates to RCSensorFusion
    [locationManager startLocationUpdates]; // Asynchronously gets the device's location and stores it
    [sensorFusion startInertialOnlyFusion]; // Starting interial-only sensor fusion ahead of time lets 3DK settle into a initialized state before full sensor fusion begins
    
    connectionManager = [ConnectionManager sharedInstance]; // For the optional remote visualization tool
    [connectionManager startSearch];
    
    isStarted = false;
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    
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

- (void)startFullSensorFusion
{
    // Setting the location helps improve accuracy by adjusting for altitude and how far you are from the equator
    CLLocation *currentLocation = [locationManager getStoredLocation];
    [sensorFusion setLocation:currentLocation];


    [avSessionManager startSession]; // Starts the AV session
    [videoManager startVideoCapture]; // Starts sending video frames to RCSensorFusion
    [LicenseHelper validateLicenseAndStartProcessingVideo]; // The evalutaion license must be validated before full sensor fusion begins.
    
    statusLabel.text = @"";
}

- (void)stopFullSensorFusion
{
    [videoManager stopVideoCapture]; // Stops sending video frames to RCSensorFusion
    [sensorFusion stopProcessingVideo]; // Ends full sensor fusion
    [avSessionManager endSession]; // Stops the AV session
}

// RCSensorFusionDelegate delegate method. Called after each video frame is processed ~ 30hz.
- (void)sensorFusionDidUpdate:(RCSensorFusionData *)data
{
    if([connectionManager isConnected]) [self updateRemoteVisualization:data]; // For the optional remote visualization tool
    
    // Calculate and show the distance the device has moved from the start point
    float distanceFromStartPoint = sqrt(data.transformation.translation.x * data.transformation.translation.x + data.transformation.translation.y * data.transformation.translation.y + data.transformation.translation.z * data.transformation.translation.z);
    if (distanceFromStartPoint) distanceText.text = [NSString stringWithFormat:@"%0.3fm", distanceFromStartPoint];
}

// RCSensorFusionDelegate delegate method. Called when sensor fusion is in an error state.
- (void)sensorFusionError:(NSError *)error
{
    switch (error.code)
    {
        case RCSensorFusionErrorCodeVision:
            statusLabel.text = @"Error: The camera cannot see well enough. Could be too dark, camera blocked, or featureless scene.";
            break;
        case RCSensorFusionErrorCodeTooFast:
            statusLabel.text = @"Error: The device was moved too fast. Try moving slower and smoother.";
            break;
        case RCSensorFusionErrorCodeOther:
            statusLabel.text = @"Error: A fatal error has occured.";
            break;
        case RCSensorFusionErrorCodeLicense:
            statusLabel.text = @"Error: License was not validated before startProcessingVideo was called.";
            break;
        default:
            statusLabel.text = @"Error: Unknown.";
            break;
    }
}

// Transmits 3DK output data to the remote visualization app running on a desktop machine on the same wifi network
- (void)updateRemoteVisualization:(RCSensorFusionData *)data
{
    double time = data.timestamp / 1.0e6;
    NSMutableDictionary * packet = [[NSMutableDictionary alloc] initWithObjectsAndKeys:[NSNumber numberWithDouble:time], @"time", nil];
    NSMutableArray * features = [[NSMutableArray alloc] initWithCapacity:[data.featurePoints count]];
    for (id object in data.featurePoints)
    {
        RCFeaturePoint * p = object;
        if([p initialized])
        {
            //NSLog(@"%lld %f %f %f", p.id, p.worldPoint.x, p.worldPoint.y, p.worldPoint.z);
            NSDictionary * f = [p dictionaryRepresentation];
            [features addObject:f];
        }
    }
    [packet setObject:features forKey:@"features"];
    [packet setObject:[[data transformation] dictionaryRepresentation] forKey:@"transformation"];
    [connectionManager sendPacket:packet];
}

// Event handler for the start/stop button
- (IBAction)buttonTapped:(id)sender
{
    if (isStarted)
    {
        [self stopFullSensorFusion];
        [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
        [connectionManager disconnect];
        [connectionManager startSearch];
    }
    else
    {
        [connectionManager connect];
        [self startFullSensorFusion];
        [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    }
    isStarted = !isStarted;
}

@end
