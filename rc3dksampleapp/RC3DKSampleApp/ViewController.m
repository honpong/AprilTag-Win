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
    AVSessionManager* sessionMan;
    MotionManager* motionMan;
    LocationManager* locationMan;
    VideoManager* videoMan;
    ConnectionManager * connectionMan;
    RCSensorFusion* sensorFusion;
    bool isStarted;
}
@synthesize startStopButton, distanceText;

- (void)viewDidLoad
{
    [super viewDidLoad];

    sessionMan = [AVSessionManager sharedInstance];
    videoMan = [VideoManager sharedInstance];
    motionMan = [MotionManager sharedInstance];
    locationMan = [LocationManager sharedInstance];
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self; // Tells RCSensorFusion where to pass data to
    connectionMan = [ConnectionManager sharedInstance];

    if([sessionMan isRunning])
    {
        NSLog(@"Ending session");
        [sessionMan endSession];
    }

    [videoMan setupWithSession:sessionMan.session]; // Can cause UI to lag if called on UI thread.

    [motionMan startMotionCapture]; // Start motion capture early
    [locationMan startLocationUpdates]; // Asynchronously gets the location and stores it
    [sensorFusion startInertialOnlyFusion];
    NSLog(@"started inertial only fusion");

    isStarted = false;
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];

    [connectionMan startSearch];
}

- (void)startFullSensorFusion
{
    NSLog(@"Starting sensor fusion");
    CLLocation *currentLocation = [locationMan getStoredLocation];
    [sensorFusion setLocation:currentLocation];


    [sessionMan startSession];
    [LicenseHelper validateLicenseAndStartProcessingVideo];
    [videoMan startVideoCapture];
}

- (void)stopFullSensorFusion
{
    [videoMan stopVideoCapture];
    [sensorFusion stopProcessingVideo];
    [sessionMan endSession];
}


- (void)sendUpdate:(RCSensorFusionData *)data
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
    [connectionMan sendPacket:packet];
}

// RCSensorFusionDelegate delegate method. Called after each video frame is processed.
- (void)sensorFusionDidUpdate:(RCSensorFusionData *)data
{
    if([connectionMan isConnected])
        [self sendUpdate:data];
    
    float distanceFromStartPoint = sqrt(data.transformation.translation.x * data.transformation.translation.x + data.transformation.translation.y * data.transformation.translation.y + data.transformation.translation.z * data.transformation.translation.z);
    distanceText.text = [NSString stringWithFormat:@"%0.3fm", distanceFromStartPoint];
}

// RCSensorFusionDelegate delegate method. Called when sensor fusion is in an error state.
- (void)sensorFusionError:(NSError *)error
{
    NSLog(@"SENSOR FUSION ERROR: %i", error.code);
}

- (IBAction)startStopButtonTapped:(id)sender
{
    if (isStarted)
    {
        [connectionMan disconnect];
        [connectionMan startSearch];
        [self stopFullSensorFusion];
        [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    }
    else
    {
        [connectionMan connect];
        [self startFullSensorFusion];
        [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    }
    isStarted = !isStarted;
}

@end
