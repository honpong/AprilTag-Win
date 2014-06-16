//
//  TMAppDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMAppDelegate.h"
#import <RCCore/RCCore.h>
#ifdef __APPLE__
#include "TargetConditionals.h"
#endif
#import "TMAnalytics.h"
#import "RCCalibration1.h"
#import "TMMeasurementTypeVC.h"
#import "TMNewMeasurementVC.h"
#import "TMHistoryVC.h"

#if TARGET_IPHONE_SIMULATOR
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@implementation TMAppDelegate
{
    UINavigationController* navigationController;
    id<RCSensorDelegate> mySensorDelegate;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^
    {
        // determine default units based on locale
        NSString* locale = [[NSLocale currentLocale] localeIdentifier];
        Units defaultUnits = [locale isEqualToString:@"en_US"] ? UnitsImperial : UnitsMetric;
        
        if ([NSUserDefaults.standardUserDefaults objectForKey:PREF_UNITS] == nil)
        {
            [NSUserDefaults.standardUserDefaults setInteger:defaultUnits forKey:PREF_UNITS];
            [NSUserDefaults.standardUserDefaults synchronize];
        }
        
        NSDictionary *appDefaults = @{PREF_UNITS: @(UnitsImperial),
                                     PREF_ADD_LOCATION: @(-1),
                                     PREF_SHOW_LOCATION_EXPLANATION: @YES,
                                     PREF_LAST_TRANS_ID: @0,
                                     PREF_IS_FIRST_LAUNCH: @YES,
                                     PREF_IS_TIPS_SHOWN: @NO};
        
        [NSUserDefaults.standardUserDefaults registerDefaults:appDefaults];
    });
    
    [Flurry setSecureTransportEnabled:YES];
    [Flurry setCrashReportingEnabled:YES];
    [Flurry setDebugLogEnabled:NO];
    [Flurry setLogLevel:FlurryLogLevelNone];
    [Flurry startSession:FLURRY_KEY];
    
    navigationController = (UINavigationController*)self.window.rootViewController;
    
    BOOL calibratedFlag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_CALIBRATED];
    BOOL hasCalibration = [SENSOR_FUSION hasCalibrationData];
    if (SKIP_CALIBRATION || (calibratedFlag && hasCalibration) )
    {
        [self gotoMainViewController];
    }
    else
    {
        if (!hasCalibration) [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED];
        [self gotoCalibration];
    }
    
    return YES;
}

- (void) gotoMainViewController
{
    self.window.rootViewController = navigationController;
}

- (void) gotoCalibration
{
    [self startMotionOnlySensorFusion];
    [self startVideoSession];
    [VIDEO_MANAGER setupWithSession:SESSION_MANAGER.session];
    [VIDEO_MANAGER startVideoCapture];
    
    UIStoryboard * calibrationStoryBoard;
    calibrationStoryBoard = [UIStoryboard storyboardWithName:@"Calibration" bundle:nil];
    RCCalibration1 * calibration1 = (RCCalibration1 *)[calibrationStoryBoard instantiateInitialViewController];
    calibration1.calibrationDelegate = self;
    calibration1.sensorDelegate = mySensorDelegate;

    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = calibration1;
}

#pragma mark RCCalibrationDelegate methods

- (AVCaptureDevice*) getVideoDevice
{
    return [SESSION_MANAGER videoDevice];
}

- (id<RCVideoFrameProvider>) getVideoProvider
{
    return VIDEO_MANAGER;
}

- (void) startVideoSession
{
    [SESSION_MANAGER startSession];
}

- (void) stopVideoSession
{
    [SESSION_MANAGER endSession];
}

- (void) calibrationDidFinish
{
    LOGME
    [VIDEO_MANAGER stopVideoCapture];
    [VIDEO_MANAGER setDelegate:nil];
    [MOTION_MANAGER stopMotionCapture];
    [self stopVideoSession];
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_CALIBRATED];
    [self gotoMainViewController];
}

- (void) calibrationScreenDidAppear:(NSString *)screenName
{
    // TODO: implement analytics logging
}

- (void) calibrationDidFail:(NSError *)error
{
    DLog("Calibration failed: %@", error);
    [self gotoCalibration];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    LOGME
    [NSUserDefaults.standardUserDefaults synchronize];
    
    if ([LOCATION_MANAGER isLocationAuthorized])
    {
        // location already authorized. go ahead.
        LOCATION_MANAGER.delegate = self;
        [LOCATION_MANAGER startLocationUpdates];
    }
    else if([self shouldShowLocationExplanation])
    {
        // show explanation, then ask for authorization. if they authorize, then start updating location.
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                        message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator. If you don't want to save location data with your measurements, you can turn that off in the preferences."
                                                       delegate:self
                                              cancelButtonTitle:@"Continue"
                                              otherButtonTitles:nil];
        [alert show];
    }
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    DLog(@"MEMORY WARNING");
}

- (BOOL)shouldShowLocationExplanation
{
    if ([CLLocationManager locationServicesEnabled])
    {
        return [CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined;
    }
    else
    {
        return [NSUserDefaults.standardUserDefaults boolForKey:PREF_SHOW_LOCATION_EXPLANATION];
    }
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == 0) //the only button
    {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
        [NSUserDefaults.standardUserDefaults synchronize];
        
        if([LOCATION_MANAGER shouldAttemptLocationAuthorization])
        {
            LOCATION_MANAGER.delegate = self;
            [LOCATION_MANAGER startLocationUpdates]; // will show dialog asking user to authorize location
        }
    }
}

- (void) startMotionOnlySensorFusion
{
    LOGME
    [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
    [MOTION_MANAGER startMotionCapture];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    if ([[NSUserDefaults.standardUserDefaults objectForKey:PREF_ADD_LOCATION] isEqual:@(-1)]) // if location pref hasn't been set
    {
        [NSUserDefaults.standardUserDefaults setObject:@YES forKey:PREF_ADD_LOCATION]; // set location pref to yes
    }
    
    LOCATION_MANAGER.delegate = nil;
    [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
}

@end
