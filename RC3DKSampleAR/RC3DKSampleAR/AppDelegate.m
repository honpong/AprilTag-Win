//
//  AppDelegate.m
//  RC3DKSampleAR
//
//  Created by Eagle Jones on 10/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "AppDelegate.h"
#import "ViewController.h"
#import "LicenseHelper.h"
#import <QuickstartKit/QuickstartKit.h>

#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"

@implementation AppDelegate
{
    UIViewController * mainViewController;
    RCSensorManager* sensorManager;
    RCLocationManager * locationManager;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // set defaults for some prefs
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    
    // Set the sensorFusion license key to allow it to validate the license
    [[RCSensorFusion sharedInstance] setLicenseKey:SDK_LICENSE_KEY];
    
    // Create a sensor delegate to manage the sensors
    sensorManager = [RCSensorManager sharedInstance];
    locationManager = [RCLocationManager sharedInstance];
    
    // save a reference to the main view controller. we use this after calibration has finished.
    mainViewController = self.window.rootViewController;
    
    // determine if calibration has been done
    BOOL isCalibrated = [[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_CALIBRATED]; // gets set to YES when calibration completes
    BOOL hasStoredCalibrationData = [[RCSensorFusion sharedInstance] hasCalibrationData]; // checks if calibration data can be retrieved
    
    // if calibration hasn't been done, or can't be retrieved, start calibration
    if (!isCalibrated || !hasStoredCalibrationData)
    {
        if (!hasStoredCalibrationData) [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED]; // ensures calibration is not marked as finished until it's completely finished
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
    // presents the first of three calibration view controllers
    RCCalibration1 *calibration1 = [RCCalibration1 instantiateFromQuickstartKit];
    calibration1.calibrationDelegate = self;
    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = calibration1;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    [[NSUserDefaults standardUserDefaults] synchronize];
    [locationManager requestLocationAccessWithCompletion:^(BOOL granted) {
        if(granted) [locationManager startLocationUpdates];
    }];
    [RCAVSessionManager requestCameraAccessWithCompletion:^(BOOL granted) {
        if(!granted)
        {
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Failure"
                                                            message:@"This app won't work without camera access."
                                                           delegate:self
                                                  cancelButtonTitle:@"Cancel"
                                                  otherButtonTitles:nil];
            [alert show];
        }
    }];
    
}

#pragma mark - RCCalibrationDelegate

- (void)startMotionSensors
{
    [sensorManager startMotionSensors];
}

- (void)stopMotionSensors
{
    [sensorManager stopAllSensors];
}

- (void) calibrationDidFinish:(UIViewController*)lastViewController
{
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED]; // set a flag to indicate calibration completed
    [self gotoMainViewController];
}

@end
