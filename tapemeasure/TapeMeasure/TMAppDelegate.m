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
#import "TMLocationIntro.h"

#if TARGET_IPHONE_SIMULATOR
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@implementation TMAppDelegate
{
    UINavigationController* navigationController;
    id<RCSensorDelegate> mySensorDelegate;
    bool waitingForLocationAuthorization;
}

-(id)init
{
    if(self = [super init])
    {
        waitingForLocationAuthorization = false;
    }
    return self;
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
                                     PREF_IS_TIPS_SHOWN: @NO,
                                     PREF_SHOW_RATE_NAG: @YES,
                                     PREF_RATE_NAG_TIMESTAMP : @0,
                                     PREF_LOCATION_NAG_TIMESTAMP: @0};
        
        [NSUserDefaults.standardUserDefaults registerDefaults:appDefaults];
    });

    #ifndef ARCHIVE // for testing
//    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_FIRST_LAUNCH];
//    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED];
//    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_SHOW_RATE_NAG];
//    [NSUserDefaults.standardUserDefaults setObject:@0 forKey:PREF_RATE_NAG_TIMESTAMP];
//    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_SHOW_LOCATION_EXPLANATION];
//    [NSUserDefaults.standardUserDefaults setObject:@0 forKey:PREF_LOCATION_NAG_TIMESTAMP];

    #endif
    
    [Flurry setSecureTransportEnabled:YES];
    [Flurry setCrashReportingEnabled:YES];
    [Flurry setDebugLogEnabled:NO];
    [Flurry setLogLevel:FlurryLogLevelNone];
    [Flurry startSession:FLURRY_KEY];
    
    navigationController = (UINavigationController*)self.window.rootViewController;
    
    mySensorDelegate = SENSOR_DELEGATE;
    
    BOOL calibratedFlag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_CALIBRATED];
    BOOL hasCalibration = [SENSOR_FUSION hasCalibrationData];
    
    if([NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_FIRST_LAUNCH] && !SKIP_CALIBRATION)
    {
        [self gotoIntroScreen];
    }
    else
    {
        if ([LOCATION_MANAGER isLocationAuthorized])
        {
            // location already authorized. go ahead.
            LOCATION_MANAGER.delegate = self;
            [LOCATION_MANAGER startLocationUpdates];
        }
        
        if (SKIP_CALIBRATION || (calibratedFlag && hasCalibration) )
        {
            [self gotoMainViewController];
        }
        else
        {
            [self gotoCalibration];
        }
    }
    
    return YES;
}

- (void) gotoMainViewController
{
    self.window.rootViewController = navigationController;
}

- (void) gotoCalibration
{
    RCCalibration1 * calibration1 = [RCCalibration1 instantiateViewController];
    calibration1.calibrationDelegate = self;
    calibration1.sensorDelegate = mySensorDelegate;
    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = calibration1;
}

- (void) gotoIntroScreen
{
    TMIntroScreen* vc = (TMIntroScreen*)[navigationController.storyboard instantiateViewControllerWithIdentifier:@"IntroScreen"];
    vc.calibrationDelegate = self;
    vc.sensorDelegate = mySensorDelegate;
    self.window.rootViewController = vc;
}

- (void) gotoTutorialVideo
{
    TMLocalMoviePlayer* movieController = [navigationController.storyboard instantiateViewControllerWithIdentifier:@"Tutorial"];
    movieController.delegate = self;
    self.window.rootViewController = movieController;
}

#pragma mark - TMLocalMoviePlayerDelegate

- (void) moviePlayerDismissed
{
    [self gotoMainViewController];
}

#pragma mark - RCCalibrationDelegate methods

- (void) calibrationDidFinish
{
    LOGME
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_CALIBRATED];
    
    if ([NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_FIRST_LAUNCH])
    {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_FIRST_LAUNCH];
        [self gotoTutorialVideo];
    }
    else
    {
        [self gotoMainViewController];
    }
}

- (void) calibrationDidFail:(NSError *)error
{
    DLog("Calibration failed: %@", error);
    [self gotoCalibration];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
   LOGME
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    DLog(@"MEMORY WARNING");
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    LOGME
    
    if ([[NSUserDefaults.standardUserDefaults objectForKey:PREF_ADD_LOCATION] isEqual:@(-1)]) // if location pref hasn't been set
    {
        [NSUserDefaults.standardUserDefaults setObject:@YES forKey:PREF_ADD_LOCATION]; // set location pref to yes
    }
    
    //LOCATION_MANAGER.delegate = nil;
    [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
}

@end
