//
//  AppDelegate.m
//  TrackLinks
//
//  Created by Ben Hirashima on 11/25/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "AppDelegate.h"
#import "CATConstants.h"
#import "RCSensorDelegate.h"
#import "RCLocationManager.h"
#import "CATHttpInterceptor.h"
#import "RC3DK.h"
#import "RCCalibration1.h"
#import "RCDebugLog.h"
#import "RCMotionManager.h"

#if TARGET_IPHONE_SIMULATOR
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@implementation AppDelegate
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
    });
    
    mySensorDelegate = [SensorDelegate sharedInstance];
    
    mainViewController = self.window.rootViewController;
    
    [NSURLProtocol registerClass:[CATHttpInterceptor class]];
    
    BOOL calibratedFlag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_CALIBRATED];
    BOOL hasCalibration = [SENSOR_FUSION hasCalibrationData];
    
    if ([LOCATION_MANAGER isLocationExplicitlyAllowed])
    {
        // location already authorized. go ahead.
        LOCATION_MANAGER.delegate = self;
        [LOCATION_MANAGER startLocationUpdates];
    }
    
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

#pragma mark RCCalibrationDelegate methods

- (void) calibrationDidFinish
{
    LOGME
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_CALIBRATED];
    
    [self gotoCaptureScreen];
}

#pragma mark - RCCalibrationDelegate

- (void) calibrationScreenDidAppear:(NSString *)screenName
{
    // log to analytics if desired
}

- (void) calibrationDidFail:(NSError *)error
{
    DLog(@"Calibration failed: %@", error);
    // TODO: implement
}

#pragma mark -

- (void)applicationWillResignActive:(UIApplication *)application
{
    LOGME
    if (MOTION_MANAGER.isCapturing) [MOTION_MANAGER stopMotionCapture];
}

@end

