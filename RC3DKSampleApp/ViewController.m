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
@synthesize startStopButton;

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
        sensorFusion.delegate = self;
        
        [videoMan setupWithSession:sessionMan.session]; // Expensive. Can cause UI to lag if called on UI thread.
    });
    
    locationMan = [LocationManager sharedInstance];
    [locationMan startLocationUpdates]; //must execute on UI thread
}

- (void)startSensorFusion
{
    CLLocation *currentLocation = [locationMan getStoredLocation];
    
    [sessionMan startSession];
    [sensorFusion startSensorFusionWithLocation:currentLocation withStaticCalibration:false];
    [videoMan startVideoCapture];
    [motionMan startMotionCapture];
}

- (void)stopSensorFusion
{
    [videoMan stopVideoCapture];
    [motionMan stopMotionCapture];
    [sensorFusion stopSensorFusion];
    [sessionMan endSession];
}

- (void)sensorFusionDidUpdate:(RCSensorFusionData *)data
{
    NSLog(@"Distance moved: %fm", data.transformation.translation.x);
}

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
