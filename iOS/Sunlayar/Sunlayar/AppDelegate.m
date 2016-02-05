//
//  AppDelegate.m
//  Sunlayar
//
//  Created by Ben Hirashima on 12/29/14.
//  Copyright (c) 2014 Sunlayar. All rights reserved.
//

#import "AppDelegate.h"
#import "SLConstants.h"
#import "RCSensorManager.h"
#import "RCLocationManager.h"
#import "RC3DK.h"
#import "RCDebugLog.h"
#import "RCMotionManager.h"
#import "RCAVSessionManager.h"
#import "RCHttpInterceptor.h"
#import "SLCaptureController.h"
#import "SLAugRealityController.h"

#if TARGET_IPHONE_SIMULATOR
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@interface AppDelegate ()

@end

@implementation AppDelegate
{
    RCSensorManager* sensorManager;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    [SENSOR_FUSION setLicenseKey:@"aF9cE0B536c84aE6F500509E8aBCcC"]; // Sunlayar's evaluation license key for 3DKPlus

    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        // Register the preference defaults early.
        NSString* locale = [[NSLocale currentLocale] localeIdentifier];
        Units defaultUnits = [locale isEqualToString:@"en_US"] ? UnitsImperial : UnitsMetric;
        
        if ([NSUserDefaults.standardUserDefaults objectForKey:PREF_UNITS] == nil)
        {
            [NSUserDefaults.standardUserDefaults setInteger:defaultUnits forKey:PREF_UNITS];
            [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_USE_LOCATION];
            [NSUserDefaults.standardUserDefaults synchronize];
        }
        
        NSDictionary *appDefaults = @{PREF_UNITS: [NSNumber numberWithInt:defaultUnits],
                                      PREF_IS_CALIBRATED: @NO};
        
        [NSUserDefaults.standardUserDefaults registerDefaults:appDefaults];
    });
    
    [NSURLProtocol registerClass:[RCHttpInterceptor class]];
    
    sensorManager = [RCSensorManager sharedInstance];
    
    BOOL calibratedFlag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_CALIBRATED];
    BOOL hasCalibration = [SENSOR_FUSION hasCalibrationData];
    
    [RCAVSessionManager requestCameraAccessWithCompletion:^(BOOL granted){
        if(!granted)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"No camera access."
                                                                message:@"This app cannot function without camera access. Turn it on in Settings/Privacy/Camera."
                                                               delegate:nil
                                                      cancelButtonTitle:@"OK"
                                                      otherButtonTitles:nil];
                [alert show];
            });
        }
    }];
    
    if (SKIP_CALIBRATION || (calibratedFlag && hasCalibration) )
    {
        [self gotoCaptureScreen];
    }
    else
    {
        [self gotoCalibration];
    }
    
    return YES;
}

- (void) gotoCaptureScreen
{
    self.window.rootViewController = [SLCaptureController new];
//    self.window.rootViewController = [SLAugRealityController new]; // for testing
}

- (void) gotoCalibration
{
    UIStoryboard* calStoryboard = [UIStoryboard storyboardWithName:@"Calibration" bundle:[NSBundle mainBundle]];
    RCCalibration1* vc = [calStoryboard instantiateViewControllerWithIdentifier:@"Calibration1"];
    vc.calibrationDelegate = self;
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
}

#pragma mark RCCalibrationDelegate methods

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
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_CALIBRATED];
    
    [self gotoCaptureScreen];
}

#pragma mark -

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    if (![LOCATION_MANAGER isLocationDisallowed])
    {
        if ([LOCATION_MANAGER isLocationExplicitlyAllowed])
        {
            [LOCATION_MANAGER startLocationUpdates];
        }
        else
        {
            [LOCATION_MANAGER requestLocationAccessWithCompletion:^(BOOL granted)
             {
                 if(granted)
                 {
                     [LOCATION_MANAGER startLocationUpdates];
                 }
             }];
        }
    }
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    if (MOTION_MANAGER.isCapturing) [MOTION_MANAGER stopMotionCapture];
}

@end
