//
//  AppDelegate.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AppDelegate.h"
#import <RC3DK/RC3DK.h>
#import "ViewController.h"
#import "RCCalibration1.h"
#import "VideoManager.h"
#import "AVSessionManager.h"

#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"
#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"

@implementation AppDelegate
{
    UIViewController * mainViewController;
    VideoManager* videoManager;
    AVSessionManager* sessionManager;
    LocationManager* locationManager;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                 [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    
    locationManager = [LocationManager sharedInstance];
    sessionManager = [AVSessionManager sharedInstance];
    videoManager = [VideoManager sharedInstance];
    
    mainViewController = self.window.rootViewController;
    
    BOOL isCalibrated = [[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_CALIBRATED];
    BOOL hasStoredCalibrationData = [[RCSensorFusion sharedInstance] hasCalibrationData];
    if (!isCalibrated || !hasStoredCalibrationData)
    {
        [self gotoCalibration];
    }
    
    return YES;
}

- (void) gotoMainViewController
{
    self.window.rootViewController = mainViewController;
}

- (void) gotoCalibration
{
    [videoManager setupWithSession:sessionManager.session];
    [videoManager startVideoCapture];
    
    RCCalibration1 * vc = [RCCalibration1 instantiateViewControllerWithDelegate:self];
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
}

#pragma mark -
#pragma mark RCCalibrationDelegate methods

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

- (void) calibrationDidFinish
{
    LOGME
    [videoManager stopVideoCapture];
    [videoManager setDelegate:nil];
    [self stopVideoSession];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED];
    [self gotoMainViewController];
}

#pragma mark -

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [[NSUserDefaults standardUserDefaults] synchronize];

    [self startMotionOnlySensorFusion];

    if ([locationManager isLocationAuthorized])
    {
        // location already authorized. go ahead.
        [self startMotionOnlySensorFusion];
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
    else
    {
        // location denied. continue without it.
        [self startMotionOnlySensorFusion];
    }
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    if ([MotionManager sharedInstance].isCapturing) [[MotionManager sharedInstance] stopMotionCapture];
    if ([RCSensorFusion sharedInstance].isSensorFusionRunning) [[RCSensorFusion sharedInstance] stopSensorFusion];
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

- (void) startMotionOnlySensorFusion
{
    LOGME
    [[RCSensorFusion sharedInstance] startInertialOnlyFusion];
    [[RCSensorFusion sharedInstance] setLocation:[locationManager getStoredLocation]];
    [[MotionManager sharedInstance] startMotionCapture];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    locationManager.delegate = nil;
    if(![[RCSensorFusion sharedInstance] isSensorFusionRunning]) [self startMotionOnlySensorFusion];
    [[RCSensorFusion sharedInstance] setLocation:[locationManager getStoredLocation]];
}

@end
