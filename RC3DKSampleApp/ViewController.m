//
//  ViewController.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "ViewController.h"

@implementation ViewController
{
    AVSessionManager* sessionMan;
    MotionManager* motionMan;
    LocationManager* locationMan;
    VideoManager* videoMan;
    RCSensorFusion* sensorFusion;
    bool isStarted;
}
@synthesize startStopButton, distanceText;

- (void)viewDidLoad
{
    [super viewDidLoad];

    //register to receive notifications of pause/resume events
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(teardown)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(setup)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
}

- (void)setup
{
    sessionMan = [AVSessionManager sharedInstance];
    videoMan = [VideoManager sharedInstance];
    motionMan = [MotionManager sharedInstance];
    locationMan = [LocationManager sharedInstance];
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self; // Tells RCSensorFusion where to pass data to
    
    [videoMan setupWithSession:sessionMan.session]; // Can cause UI to lag if called on UI thread.
    
    [motionMan startMotionCapture]; // Start motion capture early
    [locationMan startLocationUpdates]; // Asynchronously gets the location and stores it
    [sensorFusion startInertialOnlyFusion];
    
    isStarted = false;
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
}

- (void)teardown
{
    [motionMan stopMotionCapture];
    [sensorFusion stopSensorFusion];
}

- (void)startSensorFusion
{
    CLLocation *currentLocation = [locationMan getStoredLocation];
    [sensorFusion setLocation:currentLocation];
    
    [sessionMan startSession];
    [sensorFusion startProcessingVideo];
    [videoMan startVideoCapture];
}

- (void)stopSensorFusion
{
    [videoMan stopVideoCapture];
    [sensorFusion stopProcessingVideo];
    [sessionMan endSession];
}

// RCSensorFusionDelegate delegate method. Called after each video frame is processed.
- (void)sensorFusionDidUpdate:(RCSensorFusionData *)data
{
    float distanceFromStartPoint = sqrt(data.transformation.translation.x * data.transformation.translation.x + data.transformation.translation.y * data.transformation.translation.y + data.transformation.translation.z * data.transformation.translation.z);
    distanceText.text = [NSString stringWithFormat:@"%fm", distanceFromStartPoint];
}

// RCSensorFusionDelegate delegate method. Called when sensor fusion is in an error state.
- (void)sensorFusionError:(RCSensorFusionError *)error
{
    NSLog(@"ERROR: %@", error.debugDescription);
}

- (IBAction)startStopButtonTapped:(id)sender
{
    if (isStarted)
    {
        [self stopSensorFusion];
        [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    }
    else
    {
        [self startSensorFusion];
        [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    }
    isStarted = !isStarted;
}

@end
