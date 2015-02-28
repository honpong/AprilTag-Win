//
//  AppDelegate.m
//  CardboardSDK-iOS
//
//

#import "AppDelegate.h"

#import "CardboardSDK/CardboardSDK.h"
#import "TreasureViewController.h"

#import "RC3DK.h"
#import "RCSensorDelegate.h"
#import "RCDebugLog.h"
#import "RCLocationManager.h"

//#import "CardboardUnity.h"
//void _unity_getFrameParameters(float *frameParameters);


@interface AppDelegate ()

@end

#define PREF_IS_CALIBRATED @"PREF_IS_CALIBRATED"
#define PREF_SHOW_LOCATION_EXPLANATION @"RC_SHOW_LOCATION_EXPLANATION"

@implementation AppDelegate
{
    TreasureViewController *cardboardViewController;
    id<RCSensorDelegate> mySensorDelegate;
    RCLocationManager * locationManager;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    self.window.backgroundColor = [UIColor whiteColor];
    [self.window makeKeyAndVisible];

    // Google's treasure example
    cardboardViewController = [TreasureViewController new];
    
//    float *frameParameters = calloc(80, sizeof(float));
//    _unity_getFrameParameters(frameParameters);
    
    // set defaults for some prefs
    NSDictionary *appDefaults = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:NO], PREF_IS_CALIBRATED,
                                 [NSNumber numberWithBool:YES], PREF_SHOW_LOCATION_EXPLANATION,
                                 nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    
    mySensorDelegate = [SensorDelegate sharedInstance];
    locationManager = [RCLocationManager sharedInstance];
    
    // determine if calibration has been done
    BOOL isCalibrated = [[NSUserDefaults standardUserDefaults] boolForKey:PREF_IS_CALIBRATED]; // gets set to YES when calibration completes
    BOOL hasStoredCalibrationData = [[RCSensorFusion sharedInstance] hasCalibrationData]; // checks if calibration data can be retrieved
    
    // if calibration hasn't been done, or can't be retrieved, start calibration
    if (!isCalibrated || !hasStoredCalibrationData)
    {
        if (!hasStoredCalibrationData) [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_IS_CALIBRATED]; // ensures calibration is not marked as finished until it's completely finished
        [self gotoCalibration];
    }
    else
    {
        [self gotoMainViewController];
    }
    
    return YES;
}

- (void) gotoMainViewController
{
    self.window.rootViewController = cardboardViewController;
}

- (void) gotoCalibration
{
    // presents the first of three calibration view controllers
    RCCalibration1 *calibration1 = [RCCalibration1 instantiateViewController];
    calibration1.calibrationDelegate = self;
    calibration1.sensorDelegate = mySensorDelegate;
    calibration1.modalPresentationStyle = UIModalPresentationFullScreen;
    self.window.rootViewController = calibration1;
}

- (void) calibrationDidFinish:(UIViewController*)lastViewController
{
    LOGME
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:PREF_IS_CALIBRATED]; // set a flag to indicate calibration completed
    [self gotoMainViewController];
}

@end
