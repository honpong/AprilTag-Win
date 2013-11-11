//
//  MPAppDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAppDelegate.h"
#import "GAI.h"
#import "MPPhotoRequest.h"

#ifdef DEBUG
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@implementation MPAppDelegate
{
    UIAlertView *locationAlert;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        // Register the preference defaults early.
        NSString* locale = [[NSLocale currentLocale] localeIdentifier];
        Units defaultUnits = [locale isEqualToString:@"en_US"] ? UnitsImperial : UnitsMetric;
        
        if ([[NSUserDefaults standardUserDefaults] objectForKey:PREF_UNITS] == nil)
        {
            [[NSUserDefaults standardUserDefaults] setInteger:defaultUnits forKey:PREF_UNITS];
            [[NSUserDefaults standardUserDefaults] synchronize];
        }
        
        NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithInt:defaultUnits], PREF_UNITS,
                                    [NSNumber numberWithBool:YES], PREF_ADD_LOCATION,
                                    [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                    [NSNumber numberWithInt:0], PREF_LAST_TRANS_ID,
                                    [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                    [NSNumber numberWithInt:0], PREF_TUTORIAL_ANSWER,
                                    [NSNumber numberWithBool:YES], PREF_SHOW_INSTRUCTIONS,
                                    [NSNumber numberWithBool:YES], PREF_SHOW_ACCURACY_QUESTION,
                                    [NSNumber numberWithBool:YES], PREF_IS_FIRST_START,
                                    nil];
       
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
        
        if ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_FIRST_START])
        {
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_IS_FIRST_START];
        }
        
        [RCHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
    });
    
    if (SKIP_CALIBRATION || ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_CALIBRATED] && [SENSOR_FUSION hasCalibrationData]) )
    {
        MPMeasuredPhotoVC* mp = [self.window.rootViewController.storyboard instantiateViewControllerWithIdentifier:@"MeasuredPhoto"];
        self.window.rootViewController = mp;
    }
    
    // google analytics setup
    #ifndef ARCHIVE
    [GAI sharedInstance].dryRun = YES; // turns off analtyics if not an archive build
    #endif
//    [[[GAI sharedInstance] logger] setLogLevel:kGAILogLevelVerbose];
    [GAI sharedInstance].trackUncaughtExceptions = YES;
    [MPAnalytics getTracker]; // initializes tracker
    
    return YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [[NSUserDefaults standardUserDefaults] synchronize];

    [self startMotionOnlySensorFusion];

    if ([LOCATION_MANAGER isLocationAuthorized])
    {
        // location already authorized. go ahead.
        LOCATION_MANAGER.delegate = self;
        [LOCATION_MANAGER startLocationUpdates];
    }
    else if([self shouldShowLocationExplanation] && locationAlert == nil)
    {
        // show explanation, then ask for authorization. if they authorize, then start updating location.
        locationAlert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                   message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator."
                                                  delegate:self
                                         cancelButtonTitle:@"Continue"
                                         otherButtonTitles:nil];
        [locationAlert show];
    }
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    if (MOTION_MANAGER.isCapturing) [MOTION_MANAGER stopMotionCapture];
    if (SENSOR_FUSION.isSensorFusionRunning) [SENSOR_FUSION stopSensorFusion];
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
    locationAlert = nil;
}

- (void) startMotionOnlySensorFusion
{
    LOGME
    
    [SENSOR_FUSION startInertialOnlyFusion];
    [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
    [MOTION_MANAGER startMotionCapture];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    LOCATION_MANAGER.delegate = nil;
    [self startMotionOnlySensorFusion]; //This will update the location; we need to potentially restart sensor fusion because the location permission dialog pauses our app
}

// this gets called when another app requests a measured photo
- (BOOL) application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    DLog(@"Application launched with URL: %@", url);
//    [MPPhotoRequest setLastRequest:url withSourceApp:sourceApplication];
    return YES;
}

@end
