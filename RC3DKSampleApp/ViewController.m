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
}
@synthesize startStopButton, distanceText;

- (void)viewDidLoad
{
    [super viewDidLoad];
	[self setup];
}

- (void)setup
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        sessionMan = [AVSessionManager sharedInstance];
        motionMan = [MotionManager sharedInstance];
        videoMan = [VideoManager sharedInstance];
        sensorFusion = [RCSensorFusion sharedInstance];
        sensorFusion.delegate = self; // Tells RCSensorFusion where to pass data to
        
        [videoMan setupWithSession:sessionMan.session]; // Can cause UI to lag if called on UI thread.
    });
    
    [motionMan startMotionCapture];
    
    locationMan = [LocationManager sharedInstance];
    [locationMan startLocationUpdates]; //must execute on UI thread
}

- (void)startSensorFusion
{
    CLLocation *currentLocation = [locationMan getStoredLocation];
    
    [sessionMan startSession];
    [sensorFusion startSensorFusionWithLocation:currentLocation withStaticCalibration:false];
    [videoMan startVideoCapture];
}

- (void)stopSensorFusion
{
    [videoMan stopVideoCapture];
    [motionMan stopMotionCapture];
    [sensorFusion stopSensorFusion];
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
    if (sensorFusion.isSensorFusionRunning)
    {
        [self stopSensorFusion];
        [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    }
    else
    {
        [self startSensorFusion];
        [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    }
}

@end
