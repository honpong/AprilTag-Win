//
//  MPAppDelegate.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAppDelegate.h"
#import "MPPhotoRequest.h"
#ifdef __APPLE__
#include "TargetConditionals.h"
#endif
#import "MPAnalytics.h"
#import "CoreData+MagicalRecord.h"
#import "MPEditPhoto.h"
#import "MPHttpInterceptor.h"
#import "MPIntroScreen.h"
#import "Flurry.h"

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
                                    PREF_SHOW_LOCATION_NAG: @YES,
                                    PREF_LAST_TRANS_ID: @0,
                                    PREF_IS_CALIBRATED: @NO,
                                    PREF_TUTORIAL_ANSWER: @0,
                                    PREF_SHOW_INSTRUCTIONS: @YES,
                                    PREF_SHOW_ACCURACY_QUESTION: @NO, // TODO: change to YES for prod
                                    PREF_IS_FIRST_START: @YES,
                                    PREF_RATE_NAG_TIMESTAMP : @0,
                                    PREF_LOCATION_NAG_TIMESTAMP: @0,};
       
        [NSUserDefaults.standardUserDefaults registerDefaults:appDefaults];
        
        // for testing only
        #ifndef ARCHIVE
//        [NSUserDefaults.standardUserDefaults setObject:@YES forKey:PREF_IS_FIRST_START];
        #endif
        
        [RCHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
    });
    
    mySensorDelegate = [SensorDelegate sharedInstance];
    
    mainViewController = self.window.rootViewController;

    [Flurry setSecureTransportEnabled:YES];
    [Flurry setCrashReportingEnabled:YES];
    [Flurry setDebugLogEnabled:NO];
    [Flurry setLogLevel:FlurryLogLevelNone];
    [Flurry startSession:FLURRY_KEY];
    
    MagicalRecord.loggingLevel = MagicalRecordLoggingLevelVerbose;
    [MagicalRecord setupCoreDataStackWithStoreNamed:@"Model"];
    
    [NSURLProtocol registerClass:[MPHttpInterceptor class]];
    
    BOOL calibratedFlag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_CALIBRATED];
    BOOL hasCalibration = [SENSOR_FUSION hasCalibrationData];
    
    if([NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_FIRST_START] && !SKIP_CALIBRATION)
    {
        [self gotoIntroScreen];
    }
    else
    {
        if ([LOCATION_MANAGER isLocationExplicitlyAllowed])
        {
            // location already authorized. go ahead.
            LOCATION_MANAGER.delegate = self;
            [LOCATION_MANAGER startLocationUpdates];
        }
        
        if (SKIP_CALIBRATION || (calibratedFlag && hasCalibration) )
        {
            [self gotoGallery];
        }
        else
        {
            [self gotoCalibration];
        }
    }
    
    return YES;
}

- (void) gotoGallery
{
    self.window.rootViewController = mainViewController;
}

- (void) gotoCalibration
{
    RCCalibration1 * vc = [RCCalibration1 instantiateViewController];
    vc.calibrationDelegate = self;
    vc.sensorDelegate = mySensorDelegate;
    vc.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = vc;
}

- (void) gotoIntroScreen
{
    MPIntroScreen* vc = [mainViewController.storyboard instantiateViewControllerWithIdentifier:@"IntroScreen"];
    vc.calibrationDelegate = self;
    vc.sensorDelegate = mySensorDelegate;
    self.window.rootViewController = vc;
}

- (void) gotoTutorialVideo
{
    MPLocalMoviePlayer* movieController = [mainViewController.storyboard instantiateViewControllerWithIdentifier:@"Tutorial"];
    movieController.delegate = self;
    self.window.rootViewController = movieController;
}

#pragma mark RCCalibrationDelegate methods

- (void) calibrationDidFinish
{
    LOGME
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_CALIBRATED];
    
    if ([NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_FIRST_START])
    {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_FIRST_START];
        [self gotoTutorialVideo];
    }
    else
    {
        [self gotoGallery];
    }
}

#pragma mark - TMLocalMoviePlayerDelegate

- (void) moviePlayerDismissed
{
    [self gotoGallery];
}

- (void) moviePlayBackDidFinish
{
    [MPAnalytics logEvent:@"View.Tutorial.MovieFinished"];
}

#pragma mark - RCCalibrationDelegate

- (void) calibrationScreenDidAppear:(NSString *)screenName
{
    if ([screenName isEqualToString:@"Calibration1"])
        [MPAnalytics logEvent:@"View.Calibration1"];
    else if ([screenName isEqualToString:@"Calibration2"])
        [MPAnalytics logEvent:@"View.Calibration2"];
    else if ([screenName isEqualToString:@"Calibration3"])
        [MPAnalytics logEvent:@"View.Calibration3"];
}

- (void) calibrationDidFail:(NSError *)error
{
    DLog(@"Calibration failed: %@", error);
    // TODO: implement
}

#pragma mark -

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    if (MOTION_MANAGER.isCapturing) [MOTION_MANAGER stopMotionCapture];
}

- (void) applicationWillTerminate:(UIApplication *)application
{
    [MagicalRecord cleanUp];
}

// this gets called when another app requests a measured photo
- (BOOL) application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    DLog(@"Application launched with URL: %@", url);
//    [MPPhotoRequest setLastRequest:url withSourceApp:sourceApplication];
    return YES;
}

@end
