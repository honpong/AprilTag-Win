//
//  AppDelegate.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AppDelegate.h"
#import "ViewController.h"
#import "LicenseHelper.h"
#import "RCSensorDelegate.h"

#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"
#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"

@implementation AppDelegate
{
    UIViewController * mainViewController;
    id<RCSensorDelegate> mySensorDelegate;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // set defaults for some prefs
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                 [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];

    // Create a sensor delegate to manage the sensors
    mySensorDelegate = [[SensorDelegate alloc] init];

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
    UIStoryboard * calibrationStoryBoard;
    calibrationStoryBoard = [UIStoryboard storyboardWithName:@"Calibration" bundle:nil];
    RCCalibration1 * calibration1 = (RCCalibration1 *)[calibrationStoryBoard instantiateInitialViewController];
    calibration1.calibrationDelegate = self;
    calibration1.sensorDelegate = mySensorDelegate;

    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = calibration1;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [[NSUserDefaults standardUserDefaults] synchronize];
    [mySensorDelegate startLocationUpdates];
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    [mySensorDelegate stopAllSensors];
    if ([RCSensorFusion sharedInstance].isSensorFusionRunning) [[RCSensorFusion sharedInstance] stopSensorFusion];
}

- (void) calibrationScreenDidAppear:(NSString *)screenName
{
    // the license must be validated before each sensor fusion session, so validate the license key before each calibration step
    [[RCSensorFusion sharedInstance] validateLicense:API_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        if(licenseStatus != RCLicenseStatusOK) [LicenseHelper showLicenseStatusError:licenseStatus];
    } withErrorBlock:^(NSError * error) {
        [LicenseHelper showLicenseValidationError:error];
    }];
}

- (void) calibrationDidFinish
{
    LOGME
    [mySensorDelegate stopAllSensors];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED]; // set a flag to indicate calibration completed
    [self gotoMainViewController];
}

@end
