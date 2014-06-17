//
//  RCSensorDelegate.m
//
//  Created by Eagle Jones on 6/13/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCSensorDelegate.h"
#import "RCVideoManager.h"
#import "RCAVSessionManager.h"
#import "RCLocationManager.h"
#import "RCMotionManager.h"

@implementation SensorDelegate
{
    RCVideoManager* videoManager;
    RCAVSessionManager* sessionManager;
    RCLocationManager* locationManager;
    RCMotionManager* motionManager;
}

+ (id) sharedInstance
{
    static SensorDelegate *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [SensorDelegate new];
    });
    return instance;
}

- (id) init
{
    self = [super init];
    if(self)
    {
        // get references to sensor managers
        locationManager = [RCLocationManager sharedInstance];
        sessionManager = [RCAVSessionManager sharedInstance];
        videoManager = [RCVideoManager sharedInstance];
        motionManager = [RCMotionManager sharedInstance];
    }
    return self;
}

- (void) startAllSensors
{
    [self startVideoSession];
    [videoManager setupWithSession:sessionManager.session];
    [videoManager startVideoCapture];
    [motionManager startMotionCapture];
}

- (void) stopAllSensors
{
    [motionManager stopMotionCapture];
    [videoManager stopVideoCapture];
    [self stopVideoSession];
}

- (void) startMotionSensors
{
    [motionManager startMotionCapture];
}

- (void) startLocationUpdatesIfAllowed
{
    //Only attempt to start location updates if authorized or haven't yet asked
    if ([locationManager isLocationAuthorized] || [locationManager shouldAttemptLocationAuthorization])
    {
        locationManager.delegate = self;
        [locationManager startLocationUpdates];
    }
}

- (CLLocation *) getStoredLocation
{
    return [locationManager getStoredLocation];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    locationManager.delegate = nil;
    [[RCSensorFusion sharedInstance] setLocation:[locationManager getStoredLocation]];
}

- (AVCaptureDevice*) getVideoDevice
{
    return [sessionManager videoDevice];
}

- (id<RCVideoFrameProvider>) getVideoProvider
{
    return videoManager;
}

- (void) startVideoSession
{
    [sessionManager startSession];
}

- (void) stopVideoSession
{
    [sessionManager endSession];
}

@end
