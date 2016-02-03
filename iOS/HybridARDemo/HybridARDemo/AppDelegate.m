//
//  AppDelegate.m
//  HybridARDemo
//
//  Created by Ben Hirashima on 5/19/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import "AppDelegate.h"
#import "RC3DK.h"
#import "RCHttpInterceptor.h"
#import "RCConstants.h"
#import "RCMainViewController.h"
#import "LicenseHelper.h"

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
    
    // Set the sensorFusion license key to allow it to validate the license
    [[RCSensorFusion sharedInstance] setLicenseKey:SDK_LICENSE_KEY];
    
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
        [self gotoMainScreen];
    }
    else
    {
        [self gotoCalibration];
    }
    
    return YES;
}

- (void) gotoMainScreen
{
    self.window.rootViewController = [RCMainViewController new];
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
    
    [self gotoMainScreen];
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
