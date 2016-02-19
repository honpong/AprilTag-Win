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
#import "TMMeasurementTypeVC.h"
#import "TMNewMeasurementVC.h"
#import "TMHistoryVC.h"
#import "TMLocationIntro.h"
#import "RCCalibration2.h"
#import "RCCalibration3.h"

#if TARGET_IPHONE_SIMULATOR
#define SKIP_CALIBRATION YES // skip calibration when running on emulator because it cannot calibrate
#else
#define SKIP_CALIBRATION NO
#endif

@implementation TMAppDelegate
{
    UINavigationController* navigationController;
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
                                 PREF_SHOW_LOCATION_NAG: @YES,
                                 PREF_LAST_TRANS_ID: @0,
                                 PREF_IS_FIRST_LAUNCH: @YES,
                                 PREF_SHOW_RATE_NAG: @YES,
                                 PREF_RATE_NAG_TIMESTAMP : @0,
                                 PREF_LOCATION_NAG_TIMESTAMP: @0,
                                 PREF_LAST_TIP_INDEX: @(-1)};
    
    [NSUserDefaults.standardUserDefaults registerDefaults:appDefaults];
    
    if ([NSUserDefaults.standardUserDefaults objectForKey:PREF_USE_LOCATION] == nil)
    {
        // this handles the case of an upgrade from an older version that didn't have this pref.
        [NSUserDefaults.standardUserDefaults setBool:[LOCATION_MANAGER isLocationExplicitlyAllowed] forKey:PREF_USE_LOCATION];
    }

    #ifndef ARCHIVE // for testing
//    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_IS_FIRST_LAUNCH];
//    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED];
//    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_SHOW_RATE_NAG];
//    [NSUserDefaults.standardUserDefaults setObject:@0 forKey:PREF_RATE_NAG_TIMESTAMP];
//    [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_SHOW_LOCATION_NAG];
//    [NSUserDefaults.standardUserDefaults setObject:@0 forKey:PREF_LOCATION_NAG_TIMESTAMP];
    #endif
    
    #if TARGET_IPHONE_SIMULATOR // generate some measurements for testing in the simulator
    if ([TMMeasurement getAllExceptDeleted].count == 0) [self generateDummyMeasurements];
    #endif
    
    [Flurry setSecureTransportEnabled:YES];
    [Flurry setCrashReportingEnabled:YES];
    [Flurry setDebugLogEnabled:NO];
    [Flurry setLogLevel:FlurryLogLevelNone];
    [Flurry startSession:FLURRY_KEY];
    
    navigationController = (UINavigationController*)self.window.rootViewController;
    
    BOOL calibratedFlag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_CALIBRATED];
    BOOL hasCalibration = [SENSOR_FUSION hasCalibrationData];
    
    if([NSUserDefaults.standardUserDefaults boolForKey:PREF_IS_FIRST_LAUNCH])
    {
        [self gotoIntroScreen];
    }
    else
    {
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
    UIStoryboard* calStoryboard = [UIStoryboard storyboardWithName:@"Calibration" bundle:[NSBundle mainBundle]];
    RCCalibration1 * calibration1 = [calStoryboard instantiateViewControllerWithIdentifier:@"Calibration1"];
    calibration1.calibrationDelegate = self;
    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = calibration1;
}

- (void) gotoIntroScreen
{
    TMIntroScreen* vc = (TMIntroScreen*)[navigationController.storyboard instantiateViewControllerWithIdentifier:@"IntroScreen"];
    vc.calibrationDelegate = self;
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

- (void) moviePlayBackDidFinish
{
    [TMAnalytics logEvent:@"View.Tutorial.MovieFinished"];
}

#pragma mark - RCCalibrationDelegate

- (void)startMotionSensors
{
    [MOTION_MANAGER startMotionCapture];
}

- (void)stopMotionSensors
{
    [MOTION_MANAGER stopMotionCapture];
}

- (void) calibrationDidFinish:(UIViewController*)lastViewController
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

- (void) calibrationScreenDidAppear:(UIViewController*)lastViewController
{
    if ([lastViewController isKindOfClass:[RCCalibration1 class]])
        [TMAnalytics logEvent:@"View.Calibration1"];
    else if ([lastViewController isKindOfClass:[RCCalibration2 class]])
        [TMAnalytics logEvent:@"View.Calibration2"];
    else if ([lastViewController isKindOfClass:[RCCalibration3 class]])
        [TMAnalytics logEvent:@"View.Calibration3"];
}

#pragma mark -

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    LOGME
    
    BOOL useLocation = [NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION];
    BOOL locationAllowed = [LOCATION_MANAGER isLocationExplicitlyAllowed];
    if (locationAllowed && useLocation)
    {
        // location already authorized. go ahead.
        LOCATION_MANAGER.delegate = self;
        [LOCATION_MANAGER startLocationUpdates];
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

- (void) generateDummyMeasurements
{
    TMMeasurement* newMeasurement;
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.name = @"Hallway";
    [newMeasurement setPointToPoint:3.82];
    newMeasurement.timestamp = [[NSDate date] timeIntervalSince1970];
    [newMeasurement insertIntoDb];
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.name = @"Hallway";
    [newMeasurement setPointToPoint:3.83];
    newMeasurement.timestamp = [[NSDate date] timeIntervalSince1970];
    [newMeasurement insertIntoDb];
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.name = @"Paper";
    [newMeasurement setPointToPoint:0.2667];
    newMeasurement.timestamp = [[NSDate date] timeIntervalSince1970];
    [newMeasurement autoSelectUnitsScale];
    [newMeasurement insertIntoDb];
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.name = @"Laptop";
    [newMeasurement setPointToPoint:0.3302];
    newMeasurement.timestamp = [[NSDate date] timeIntervalSince1970];
    newMeasurement.unitsScaleImperial = UnitsScaleIN;
    [newMeasurement insertIntoDb];
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.name = @"Table";
    [newMeasurement setPointToPoint:1.01];
    newMeasurement.units = UnitsMetric;
    newMeasurement.timestamp = [[NSDate date] timeIntervalSince1970];
    [newMeasurement insertIntoDb];
    
    TMLocation* location = [TMLocation getNewLocation];
    location.locationName = @"Home";
    [location insertIntoDb];
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.name = @"Driveway";
    [newMeasurement setPointToPoint:100];
    newMeasurement.timestamp = [[NSDate date] timeIntervalSince1970];
    [newMeasurement insertIntoDb];
    
    [location addMeasurementObject:newMeasurement];
    
    [DATA_MANAGER saveContext];
}

@end
