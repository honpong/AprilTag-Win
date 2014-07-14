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
#ifdef __APPLE__
#include "TargetConditionals.h"
#endif
#import "MPAnalytics.h"
#import "CoreData+MagicalRecord.h"
#import "MPEditPhoto.h"
#import "MPHttpUrlProtocol.h"

#if TARGET_IPHONE_SIMULATOR
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@implementation MPAppDelegate
{
    UIAlertView *locationAlert;
    UIViewController* mainViewController;
    id<RCSensorDelegate> mySensorDelegate;
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
            [NSUserDefaults.standardUserDefaults synchronize];
        }
        
        NSDictionary *appDefaults = @{PREF_UNITS: [NSNumber numberWithInt:defaultUnits],
                                    PREF_ADD_LOCATION: @YES,
                                    PREF_SHOW_LOCATION_EXPLANATION: @YES,
                                    PREF_LAST_TRANS_ID: @0,
                                    PREF_IS_CALIBRATED: @NO,
                                    PREF_TUTORIAL_ANSWER: @0,
                                    PREF_SHOW_INSTRUCTIONS: @YES,
                                    PREF_SHOW_ACCURACY_QUESTION: @NO, // TODO: change to YES for prod
                                    PREF_IS_FIRST_START: @YES};
       
        [NSUserDefaults.standardUserDefaults registerDefaults:appDefaults];
         
//        [NSUserDefaults.standardUserDefaults setObject:@YES forKey:PREF_IS_FIRST_START]; // temp, for testing
        
        [RCHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
    });
    
    mySensorDelegate = [SensorDelegate sharedInstance];
    
    mainViewController = self.window.rootViewController;
    
    BOOL calibratedFlag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_CALIBRATED];
    BOOL hasCalibration = [SENSOR_FUSION hasCalibrationData];
    if (SKIP_CALIBRATION || (calibratedFlag && hasCalibration))
    {
        [self gotoCapturePhoto];
    }
    else
    {
        if (!hasCalibration) [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED];
        [self gotoCalibration];
    }
    
    // google analytics setup
    #ifndef ARCHIVE
    [GAI sharedInstance].dryRun = YES; // turns off analtyics if not an archive build
    #endif
//    [[[GAI sharedInstance] logger] setLogLevel:kGAILogLevelVerbose];
    [GAI sharedInstance].trackUncaughtExceptions = YES;
    [MPAnalytics getTracker]; // initializes tracker
    
    MagicalRecord.loggingLevel = MagicalRecordLoggingLevelVerbose;
    [MagicalRecord setupCoreDataStackWithStoreNamed:@"Model"];
    
    [NSURLProtocol registerClass:[MPHttpUrlProtocol class]];
    
    return YES;
}

- (void) gotoCapturePhoto
{
//    self.window.rootViewController = mainViewController;
    MPEditPhoto* editPhotoController = [MPEditPhoto new];
    self.window.rootViewController = editPhotoController;
}

- (void) gotoCalibration
{
    RCCalibration1 * vc = [RCCalibration1 instantiateViewController];
    vc.calibrationDelegate = self;
    vc.sensorDelegate = mySensorDelegate;
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
}

#pragma mark RCCalibrationDelegate methods

- (void) calibrationDidFinish
{
    LOGME
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_CALIBRATED];
    [self gotoCapturePhoto];
}

- (void) calibrationScreenDidAppear:(NSString *)screenName
{
    [MPAnalytics logScreenView:screenName];
}

- (void) calibrationDidFail:(NSError *)error
{
    DLog("Calibration failed: %@", error);
    // TODO: implement
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    [NSUserDefaults.standardUserDefaults synchronize];

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
    [MagicalRecord cleanUp];
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
    locationAlert = nil;
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    LOCATION_MANAGER.delegate = nil;
    [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
}

// this gets called when another app requests a measured photo
- (BOOL) application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    DLog(@"Application launched with URL: %@", url);
//    [MPPhotoRequest setLastRequest:url withSourceApp:sourceApplication];
    return YES;
}

@end
