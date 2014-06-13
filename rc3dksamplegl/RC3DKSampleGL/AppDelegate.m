//
//  AppDelegate.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AppDelegate.h"
#import "VisualizationController.h"
#import "RCMotionManager.h"
#import "RCVideoManager.h"
#import "RCAVSessionManager.h"
#import "LicenseHelper.h"

#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"
#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"

@implementation AppDelegate
{
    UIViewController * mainViewController;
    RCVideoManager* videoManager;
    RCAVSessionManager* sessionManager;
    RCLocationManager* locationManager;
    RCMotionManager* motionManager;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // set defaults for some prefs
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                 [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    
    // get references to sensor managers
    locationManager = [RCLocationManager sharedInstance];
    sessionManager = [RCAVSessionManager sharedInstance];
    videoManager = [RCVideoManager sharedInstance];
    motionManager = [RCMotionManager sharedInstance];
    
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
    // start video and motion capture. we stop it in calibrationDidFinish: below.
    [motionManager startMotionCapture];
    [self startVideoSession];
    [videoManager setupWithSession:sessionManager.session];
    [videoManager startVideoCapture];
    
    // presents the first of three calibration view controllers
    RCCalibration1 * vc = [RCCalibration1 instantiateViewControllerWithDelegate:self];
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [[NSUserDefaults standardUserDefaults] synchronize];
    
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

- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    if ([RCMotionManager sharedInstance].isCapturing) [[RCMotionManager sharedInstance] stopMotionCapture];
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

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    locationManager.delegate = nil;
    [[RCSensorFusion sharedInstance] setLocation:[locationManager getStoredLocation]];
}

#pragma mark - RCCalibrationDelegate methods

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
    [motionManager stopMotionCapture];
    [videoManager stopVideoCapture];
    [videoManager setDelegate:nil];
    [self stopVideoSession];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED]; // set a flag to indicate calibration completed
    [self gotoMainViewController];
}

@end
