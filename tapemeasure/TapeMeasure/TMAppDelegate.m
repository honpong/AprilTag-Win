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

#if TARGET_IPHONE_SIMULATOR
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@implementation TMAppDelegate
{
    UIViewController* mainViewController;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^
    {
        // determine default units based on locale
        NSString* locale = [[NSLocale currentLocale] localeIdentifier];
        Units defaultUnits = [locale isEqualToString:@"en_US"] ? UnitsImperial : UnitsMetric;
        
        if ([[NSUserDefaults standardUserDefaults] objectForKey:PREF_UNITS] == nil)
        {
            [[NSUserDefaults standardUserDefaults] setInteger:defaultUnits forKey:PREF_UNITS];
            [[NSUserDefaults standardUserDefaults] synchronize];
        }
        
        NSDictionary *appDefaults = @{PREF_UNITS: @(UnitsImperial),
                                     PREF_ADD_LOCATION: @YES,
                                     PREF_SHOW_LOCATION_EXPLANATION: @YES,
                                     PREF_LAST_TRANS_ID: @0};
        
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
        
        NSSetUncaughtExceptionHandler(&uncaughtExceptionHandler);
        [Flurry startSession:FLURRY_KEY];
    });
    
    mainViewController = self.window.rootViewController;
    
    if (SKIP_CALIBRATION || ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_CALIBRATED] && [SENSOR_FUSION hasCalibrationData]) )
    {
        [self gotoMainViewController];
    }
    else
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
    [VIDEO_MANAGER setupWithSession:SESSION_MANAGER.session];
    [VIDEO_MANAGER startVideoCapture];
    
    RCCalibration1 * vc = [RCCalibration1 instantiateViewControllerWithDelegate:self];
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
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
    [self stopVideoSession];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED];
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
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    if ([LOCATION_MANAGER isLocationAuthorized])
    {
        // location already authorized. go ahead.
        [self startMotionOnlySensorFusion];
        LOCATION_MANAGER.delegate = self;
        [LOCATION_MANAGER startLocationUpdates];
    }
    else if([self shouldShowLocationExplanation])
    {
        // show explanation, then ask for authorization. if they authorize, then start updating location.
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                        message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator. If you don't want us to save your location data after the measurement, you can turn that off in the settings."
                                                       delegate:self
                                              cancelButtonTitle:@"Continue"
                                              otherButtonTitles:nil];
        [alert show];
    } else {
        //not authorized; just start sensor fusion.
        [self startMotionOnlySensorFusion];
    }
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    LOGME
    [MOTION_MANAGER stopMotionCapture];
    [SENSOR_FUSION stopSensorFusion];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    DLog(@"MEMORY WARNING");
}

void uncaughtExceptionHandler(NSException *exception)
{
    [Flurry logError:@"UncaughtException" message:exception.debugDescription exception:exception];
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
    if(![SENSOR_FUSION isSensorFusionRunning]) [SENSOR_FUSION startInertialOnlyFusion];
    [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
    [MOTION_MANAGER startMotionCapture];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    LOCATION_MANAGER.delegate = nil;
    [self startMotionOnlySensorFusion];
    [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
}

@end
