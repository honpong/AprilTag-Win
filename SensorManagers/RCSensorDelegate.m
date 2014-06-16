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

#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"

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
        //set default for location explanation
        NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                     [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                     nil];
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];

        
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

- (BOOL)shouldShowLocationExplanation
{
    if ([CLLocationManager locationServicesEnabled])
    {
        return [CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined;
    }
    else
    {
        return [[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_LOCATION_EXPLANATION];
    }
}

- (void) startLocationUpdates
{
    if ([locationManager isLocationAuthorized])
    {
        // location already authorized. go ahead.
        locationManager.delegate = self;
        [locationManager startLocationUpdates];
    }
    else if([self shouldShowLocationExplanation])
    {
        // show explanation, then ask for authorization. if they authorize, then start updating location.
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                        message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator."
                                                       delegate:self
                                              cancelButtonTitle:@"Continue"
                                              otherButtonTitles:nil];
        [alert show];
    }
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 0) //the only button
    {
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
        [[NSUserDefaults standardUserDefaults] synchronize];
        
        if([locationManager shouldAttemptLocationAuthorization])
        {
            locationManager.delegate = self;
            [locationManager startLocationUpdates]; // will show dialog asking user to authorize location
        }
    }
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
