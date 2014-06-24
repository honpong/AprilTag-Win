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
#import "RCSensorDelegate.h"

#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"
#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"

@implementation AppDelegate
{
    UIViewController * mainViewController;
    id<RCSensorDelegate> mySensorDelegate;
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
    mySensorDelegate = [SensorDelegate sharedInstance];
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
    RCCalibration1 *calibration1 = [RCCalibration1 instantiateViewController];
    calibration1.calibrationDelegate = self;
    calibration1.sensorDelegate = mySensorDelegate;
    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = calibration1;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [[NSUserDefaults standardUserDefaults] synchronize];

    if([self shouldShowLocationExplanation])
    {
        //On first launch, show explanation for why we need location. The alert view's callback will attempt to start location, which will ask for authorization.
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                        message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator."
                                                       delegate:self
                                              cancelButtonTitle:@"Continue"
                                              otherButtonTitles:nil];
        [alert show];
    }
    else
    {
        //Does nothing if location is not authorized. The SensorDelegate calls [RCSensorFusion setLocation] automatically. If you do not use the SensorDelegate, make sure you pass the location to RCSensorFusion before starting sensor fusion.
        [mySensorDelegate startLocationUpdatesIfAllowed];
    }
}

- (BOOL)shouldShowLocationExplanation
{
    return [[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_LOCATION_EXPLANATION];
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 0) //the only button
    {
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
        [[NSUserDefaults standardUserDefaults] synchronize];
        [mySensorDelegate startLocationUpdatesIfAllowed];
    }
}

- (void) calibrationDidFinish
{
    LOGME
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED]; // set a flag to indicate calibration completed
    [self gotoMainViewController];
}

@end
