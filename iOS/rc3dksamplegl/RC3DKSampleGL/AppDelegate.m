//
//  AppDelegate.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AppDelegate.h"
#import "VisualizationController.h"
#import "LicenseHelper.h"

#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"
#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"

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
                                 [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
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
    
    if ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_LOCATION_EXPLANATION])
    {
        [self gotoLocationPermissionScreen];
    }
    else
    {
        // if calibration hasn't been done, or can't be retrieved, start calibration
        if (!isCalibrated || !hasStoredCalibrationData)
        {
            if (!hasStoredCalibrationData) [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED]; // ensures calibration is not marked as finished until it's completely finished
            [self gotoCalibration];
        }
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

- (void) gotoLocationPermissionScreen
{
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
    
    LocationPermissionController* viewController = [mainViewController.storyboard instantiateViewControllerWithIdentifier:@"LocationPermission"];
    viewController.delegate = self;
    self.window.rootViewController = viewController;
}

#pragma mark - LocationPermissionControllerDelegate

- (void) locationPermissionResult:(BOOL)granted
{
    if(granted) [locationManager startLocationUpdates];
    
    [self gotoCalibration];
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
